/* Copyright (c) 2020 Themaister
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "replayer_driver.hpp"
#include "rdp_dump.hpp"

#include <vector>

#include "application.hpp"
#include "flat_renderer.hpp"
#include "logging.hpp"
#include "ui_manager.hpp"
#include "rdp_command_builder.hpp"
#include "stb_image.h"
#include "string_helpers.hpp"

using namespace RDP;
using namespace Granite;
using namespace Vulkan;

enum class ReplayMode
{
	VIScanout,
	DrawCall
};

enum class VisualizationMode
{
	Color,
	Depth,
	Coverage
};

struct UIMessage
{
	std::string message;
	MessageType type;
	unsigned current_lifetime;
	unsigned total_lifetime;
};

struct DebugApplication : Application, EventHandler, ReplayerEventInterface
{
	explicit DebugApplication(const std::string &path);
	void render_frame(double, double) override;
	void update_screen(const void *data, unsigned width, unsigned height, unsigned row_length) override;
	void notify_command(Op command_id, uint32_t num_words, const uint32_t *words) override;
	void message(MessageType type, const char *msg) override;
	void eof() override;
	void set_context_index(unsigned index) override;
	void signal_complete() override;

	unsigned get_default_width() override
	{
		return 1280;
	}

	unsigned get_default_height() override
	{
		return 480;
	}

	DumpPlayer dump;
	std::unique_ptr<ReplayerDriver> replayers[2];
	std::unique_ptr<ReplayerDriver> combined_replayer;
	std::string dump_path;

	void on_device_created(const DeviceCreatedEvent &e);
	void on_device_destroyed(const DeviceCreatedEvent &e);
	void on_swapchain_created(const SwapchainParameterEvent &e);
	void on_swapchain_destroyed(const SwapchainParameterEvent &e);
	bool on_key_pressed(const KeyboardEvent &e);
	bool on_mouse_move(const MouseMoveEvent &e);
	bool on_mouse_event(const MouseButtonEvent &e);
	void render_ui(CommandBuffer &cmd);
	void render_ui_vi_scanout(unsigned width, unsigned height);
	void render_ui_draw_call(unsigned width, unsigned height);
	void render_ui_messages(unsigned width, unsigned height);
	void render_ui_draw_calls(unsigned width, unsigned height);
	void render_ui_view_state(unsigned width, unsigned height);

	void render_text_top_left_down(const Font &font, int &x, int &y, const std::string &text, const vec3 &color);
	void render_text_top_right_down(const Font &font, int &x, int &y, const std::string &text, const vec3 &color);
	void render_text_bottom_right_up(const Font &font, int &x, int &y, const std::string &text, const vec3 &color);
	void render_text_bottom_left_up(const Font &font, int &x, int &y, const std::string &text, const vec3 &color);
	void render_scanout_texture(CommandBuffer &cmd);

	template <typename Op>
	void replay_until(const Op &op);

	void add_message(std::string message, MessageType type);

	struct
	{
		ReplayMode replay_mode = ReplayMode::VIScanout;
		unsigned replay_vi_frame_count = 0;
		unsigned replay_draw_count = 0;
		unsigned replay_draw_count_in_frame = 0;
		FlatRenderer flat_renderer;
		std::vector<UIMessage> current_messages;
		ImageHandle scanout_image[2];
		bool paused = false;
		bool eof = false;
		VisualizationMode vismode = VisualizationMode::Color;
		unsigned frame_step = 0;

		std::vector<Op> command_queue;
	} ui;

	struct
	{
		unsigned window_width = 0;
		unsigned window_height = 0;
		float last_mouse_x = 0;
		float last_mouse_y = 0;
		struct ZoomState
		{
			vec2 center;
			vec2 extent;
		};
		std::vector<ZoomState> state;
	} view;

	struct
	{
		std::vector<u8vec4> buffer;
		unsigned width = 0;
		unsigned height = 0;
	} cached_frame[2];

	struct
	{
		unsigned fb_format = 0;
		unsigned fb_size = 0;
		unsigned fb_width = 0;
		unsigned fb_address = 0;
	} cached_color_image;

	struct
	{
		unsigned fb_address = 0;
	} cached_depth_image;

	void zoom_in();
	void zoom_out();

	struct Rect
	{
		vec2 offset;
		vec2 size;
	};
	Rect get_texture_rect() const;
	void update_cached_frame_from_color_pointer(unsigned index);
	void update_cached_frame_from_depth_pointer(unsigned index);
	void update_cached_frame_from_coverage_pointer(unsigned index);
	void update_scanout_image_from_cached_frame(unsigned index);

	CommandBuilder builder;
	unsigned current_context_index = 0;
};

DebugApplication::DebugApplication(const std::string &path)
	: dump_path(path)
{
	get_wsi().set_backbuffer_srgb(false);

	EVENT_MANAGER_REGISTER_LATCH(DebugApplication, on_device_created, on_device_destroyed, DeviceCreatedEvent);
	EVENT_MANAGER_REGISTER_LATCH(DebugApplication, on_swapchain_created, on_swapchain_destroyed, SwapchainParameterEvent);
	EVENT_MANAGER_REGISTER(DebugApplication, on_key_pressed, KeyboardEvent);
	EVENT_MANAGER_REGISTER(DebugApplication, on_mouse_move, MouseMoveEvent);
	EVENT_MANAGER_REGISTER(DebugApplication, on_mouse_event, MouseButtonEvent);
}

void DebugApplication::set_context_index(unsigned index)
{
	current_context_index = index;
}

void DebugApplication::signal_complete()
{
}

void DebugApplication::update_scanout_image_from_cached_frame(unsigned index)
{
	if (!cached_frame[index].buffer.empty())
	{
		ImageCreateInfo info = ImageCreateInfo::immutable_2d_image(cached_frame[index].width, cached_frame[index].height,
		                                                           VK_FORMAT_R8G8B8A8_UNORM);
		ImageInitialData initial = {};
		initial.data = cached_frame[index].buffer.data();
		ui.scanout_image[index] = get_wsi().get_device().create_image(info, &initial);
	}
	else
		ui.scanout_image[index].reset();
}

void DebugApplication::update_screen(const void *data, unsigned width, unsigned height, unsigned row_length)
{
	unsigned index = current_context_index;
	if (ui.replay_mode == ReplayMode::VIScanout)
	{
		if (width && height)
		{
			cached_frame[index].buffer.resize(width * height);
			auto *src = static_cast<const u8vec4 *>(data);
			auto *dst = cached_frame[index].buffer.data();
			for (unsigned y = 0; y < height; y++, src += row_length, dst += width)
				memcpy(dst, src, width * sizeof(u8vec4));
			cached_frame[index].width = width;
			cached_frame[index].height = height;
		}
		else
		{
			cached_frame[index] = {};
		}
		update_scanout_image_from_cached_frame(index);
	}

	if (index == 0)
	{
		ui.replay_vi_frame_count++;
		ui.replay_draw_count_in_frame = 0;
	}
}

void DebugApplication::update_cached_frame_from_color_pointer(unsigned index)
{
	if (cached_color_image.fb_width == 0)
	{
		cached_frame[index] = {};
		update_scanout_image_from_cached_frame(index);
		return;
	}

	if (cached_color_image.fb_size == 2)
	{
		auto *rdram = reinterpret_cast<const uint16_t *>(replayers[index]->get_rdram());
		size_t mask = (replayers[index]->get_rdram_size() >> 1) - 1;
		size_t addr = cached_color_image.fb_address >> 1;

		cached_frame[index].width = cached_color_image.fb_width;
		// Height is not actually known statically, but assume 4:3 aspect ratio.
		cached_frame[index].height = std::max(1u, (cached_color_image.fb_width * 3) / 4);
		cached_frame[index].buffer.resize(cached_frame[index].width * cached_frame[index].height);

		for (unsigned y = 0; y < cached_frame[index].height; y++)
		{
			for (unsigned x = 0; x < cached_frame[index].width; x++)
			{
				unsigned addr_index = y * cached_frame[index].width + x;
				uint16_t color = rdram[((addr + addr_index) & mask) ^ 1]; // Endian fix-up

				auto r = (color >> 11) & 31;
				auto g = (color >> 6) & 31;
				auto b = (color >> 1) & 31;
				r = (r << 3) | (r >> 2);
				g = (g << 3) | (g >> 2);
				b = (b << 3) | (b >> 2);
				cached_frame[index].buffer[addr_index] = u8vec4(r, g, b, (color & 1) * 0xff);
			}
		}
		update_scanout_image_from_cached_frame(index);
	}
	else
	{
		fprintf(stderr, "TODO: Non-16 bit draw calls.\n");
		cached_frame[index] = {};
		update_scanout_image_from_cached_frame(index);
	}
}

void DebugApplication::update_cached_frame_from_depth_pointer(unsigned index)
{
	if (cached_color_image.fb_width == 0)
	{
		cached_frame[index] = {};
		update_scanout_image_from_cached_frame(index);
		return;
	}

	auto *rdram = reinterpret_cast<const uint16_t *>(replayers[index]->get_rdram());
	auto *hidden_rdram = reinterpret_cast<const uint8_t *>(replayers[index]->get_hidden_rdram());
	size_t mask = (replayers[index]->get_rdram_size() >> 1) - 1;
	size_t addr = cached_depth_image.fb_address >> 1;

	cached_frame[index].width = cached_color_image.fb_width;
	// Height is not actually known statically, but assume 4:3 aspect ratio.
	cached_frame[index].height = std::max(1u, (cached_color_image.fb_width * 3) / 4);
	cached_frame[index].buffer.resize(cached_frame[index].width * cached_frame[index].height);

	for (unsigned y = 0; y < cached_frame[index].height; y++)
	{
		for (unsigned x = 0; x < cached_frame[index].width; x++)
		{
			unsigned addr_index = y * cached_frame[index].width + x;
			uint16_t color = rdram[((addr + addr_index) & mask) ^ 1]; // Endian fix-up
			uint8_t dz = hidden_rdram[((addr + addr_index) & mask)];
			dz |= (color & 3) << 2;
			color &= ~3;

			auto r = (color >> 11) & 31;
			auto g = (color >> 6) & 31;
			auto b = (color >> 1) & 31;
			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);
			cached_frame[index].buffer[addr_index] = u8vec4(r, g, b, dz);
		}
	}
	update_scanout_image_from_cached_frame(index);
}

void DebugApplication::update_cached_frame_from_coverage_pointer(unsigned index)
{
	if (cached_color_image.fb_width == 0 || cached_color_image.fb_size != 2)
	{
		cached_frame[index] = {};
		update_scanout_image_from_cached_frame(index);
		return;
	}

	auto *rdram = reinterpret_cast<const uint16_t *>(replayers[index]->get_rdram());
	auto *hidden_rdram = reinterpret_cast<const uint8_t *>(replayers[index]->get_hidden_rdram());
	size_t mask = (replayers[index]->get_rdram_size() >> 1) - 1;
	size_t addr = cached_color_image.fb_address >> 1;

	cached_frame[index].width = cached_color_image.fb_width;
	// Height is not actually known statically, but assume 4:3 aspect ratio.
	cached_frame[index].height = std::max(1u, (cached_color_image.fb_width * 3) / 4);
	cached_frame[index].buffer.resize(cached_frame[index].width * cached_frame[index].height);

	for (unsigned y = 0; y < cached_frame[index].height; y++)
	{
		for (unsigned x = 0; x < cached_frame[index].width; x++)
		{
			unsigned addr_index = y * cached_frame[index].width + x;
			uint16_t color = rdram[((addr + addr_index) & mask) ^ 1]; // Endian fix-up
			uint8_t hidden_color = hidden_rdram[(addr + addr_index) & mask] & 3;

			uint8_t cov = ((color & 1) << 2) | hidden_color;
			cov = (cov << 5) | (cov << 2) | (cov >> 1);
			cached_frame[index].buffer[addr_index] = u8vec4(cov, cov, cov, 0xff);
		}
	}
	update_scanout_image_from_cached_frame(index);
}

void DebugApplication::notify_command(Op command_id, uint32_t num_words, const uint32_t *words)
{
	if (current_context_index != 0)
		return;

	if (command_is_draw_call(command_id))
	{
		ui.replay_draw_count++;
		ui.replay_draw_count_in_frame++;
	}

	if (command_id == Op::SetColorImage)
	{
		cached_color_image.fb_format = (words[0] >> 21) & 0x7;
		cached_color_image.fb_size = (words[0] >> 19) & 0x3;
		cached_color_image.fb_width = (words[0] & 0x3ff) + 1;
		cached_color_image.fb_address = words[1] & 0x00ffffff;
	}
	else if (command_id == Op::SetMaskImage)
	{
		cached_depth_image.fb_address = words[1] & 0x00ffffff;
	}

	if (ui.command_queue.size() >= 16)
		ui.command_queue.erase(ui.command_queue.begin());
	ui.command_queue.push_back(command_id);
}

void DebugApplication::message(MessageType type, const char *msg)
{
	add_message(msg, type);
}

void DebugApplication::eof()
{
	ui.eof = true;
}

bool DebugApplication::on_mouse_move(const MouseMoveEvent &e)
{
	view.last_mouse_x = float(e.get_abs_x());
	view.last_mouse_y = float(e.get_abs_y());
	return true;
}

bool DebugApplication::on_mouse_event(const MouseButtonEvent &e)
{
	view.last_mouse_x = float(e.get_abs_x());
	view.last_mouse_y = float(e.get_abs_y());

	if (e.get_pressed())
	{
		switch (e.get_button())
		{
		case MouseButton::Left:
			zoom_in();
			break;

		case MouseButton::Right:
			zoom_out();
			break;

		default:
			break;
		}
	}

	return true;
}

void DebugApplication::zoom_out()
{
	if (view.state.empty())
		return;
	view.state.pop_back();
	add_message("Zooming out!", MessageType::Info);
}

void DebugApplication::zoom_in()
{
	vec2 uv = vec2(view.last_mouse_x, view.last_mouse_y) / vec2(view.window_width, view.window_height);
	uv.x = muglm::fract(2.0f * uv.x);
	uv = 2.0f * uv - 1.0f;

	vec2 current_center = view.state.empty() ? vec2(0.5f) : view.state.back().center;
	vec2 current_extent = view.state.empty() ? vec2(0.5f) : view.state.back().extent;
	vec2 new_center = current_center + uv * current_extent;
	vec2 new_extent = current_extent * 0.8f;
	new_center = clamp(new_center, new_extent, 1.0f - new_extent);
	view.state.push_back({ new_center, new_extent });
	add_message("Zooming in!", MessageType::Info);
}

bool DebugApplication::on_key_pressed(const KeyboardEvent &e)
{
	if (e.get_key_state() == KeyState::Pressed)
	{
		switch (e.get_key())
		{
		case Key::R:
		{
			if (dump.rewind())
			{
				ui.replay_vi_frame_count = 0;
				for (auto &image : ui.scanout_image)
					image.reset();
				ui.eof = false;
				add_message("Rewind!", MessageType::Info);
			}
			else
				add_message("Failed to rewind dump!", MessageType::Error);
			break;
		}

		case Key::P:
		{
			ui.paused = !ui.paused;
			add_message(ui.paused ? "Paused!" : "Unpaused!", MessageType::Info);
			break;
		}

		case Key::Z:
		{
			ui.vismode = VisualizationMode::Depth;
			add_message("Draw depth mode", MessageType::Info);
			break;
		}

		case Key::C:
		{
			ui.vismode = VisualizationMode::Color;
			add_message("Draw color mode", MessageType::Info);
			break;
		}

		case Key::X:
		{
			ui.vismode = VisualizationMode::Coverage;
			add_message("Draw coverage mode", MessageType::Info);
			break;
		}

		case Key::_1:
			ui.frame_step = 1;
			add_message("Stepping 1 frame!", MessageType::Info);
			break;

		case Key::_2:
			ui.frame_step = 10;
			add_message("Stepping 10 frames!", MessageType::Info);
			break;

		case Key::_3:
			ui.frame_step = 100;
			add_message("Stepping 100 frames!", MessageType::Info);
			break;

		case Key::_4:
			ui.frame_step = 1000;
			add_message("Stepping 1000 frames!", MessageType::Info);
			break;

		case Key::V:
			ui.replay_mode = ReplayMode::VIScanout;
			break;

		case Key::D:
			ui.replay_mode = ReplayMode::DrawCall;
			break;

		default:
			break;
		}
	}
	return true;
}

void DebugApplication::on_device_created(const DeviceCreatedEvent &e)
{
#if 1
	if (!dump.load_dump(dump_path.c_str()))
		throw std::runtime_error("Failed to load RDP dump.");
	replayers[0] = create_replayer_driver_angrylion(dump, *this);
	replayers[1] = create_replayer_driver_parallel(e.get_device(), dump, *this);
	combined_replayer = create_side_by_side_driver(replayers[0].get(), replayers[1].get(), *this);
	dump.set_command_interface(combined_replayer.get());
#else
	replayers[0] = create_replayer_driver_angrylion(builder, *this);
	replayers[1] = create_replayer_driver_parallel(e.get_device(), builder, *this);
	combined_replayer = create_side_by_side_driver(replayers[0].get(), replayers[1].get(), *this);
	builder.set_command_interface(combined_replayer.get());
	builder.set_color_image(TextureFormat::RGBA, TextureSize::Bpp16, 0, 320);
	builder.set_depth_image(1u << 20u);
#endif
}

void DebugApplication::on_device_destroyed(const DeviceCreatedEvent &)
{
}

void DebugApplication::on_swapchain_created(const SwapchainParameterEvent &e)
{
	view.window_width = e.get_width();
	view.window_height = e.get_height();
	view.state.clear();
}

void DebugApplication::on_swapchain_destroyed(const SwapchainParameterEvent &)
{

}

DebugApplication::Rect DebugApplication::get_texture_rect() const
{
	vec2 current_center = view.state.empty() ? vec2(0.5f) : view.state.back().center;
	vec2 current_extent = view.state.empty() ? vec2(0.5f) : view.state.back().extent;
	vec2 offset = (current_center - current_extent) * vec2(cached_frame[0].width, cached_frame[0].height);
	vec2 size = current_extent * 2.0f * vec2(cached_frame[0].width, cached_frame[0].height);
	return { offset, size };
}

void DebugApplication::render_scanout_texture(CommandBuffer &cmd)
{
	if (ui.scanout_image[0])
	{
		auto rect = get_texture_rect();
		ui.flat_renderer.render_textured_quad(ui.scanout_image[0]->get_view(),
		                                      vec3(0.0f, 0.0f, 2.0f), vec2(cmd.get_viewport().width * 0.5f, cmd.get_viewport().height),
		                                      rect.offset, rect.size, DrawPipeline::Opaque, vec4(1.0f), StockSampler::NearestClamp);
	}

	if (ui.scanout_image[1])
	{
		auto rect = get_texture_rect();
		ui.flat_renderer.render_textured_quad(ui.scanout_image[1]->get_view(),
		                                      vec3(cmd.get_viewport().width * 0.5f, 0.0f, 2.0f),
		                                      vec2(cmd.get_viewport().width * 0.5f, cmd.get_viewport().height),
		                                      rect.offset, rect.size, DrawPipeline::Opaque, vec4(1.0f), StockSampler::NearestClamp);
	}
}

template <typename Op>
void DebugApplication::replay_until(const Op &op)
{
#if 1
	while (dump.iterate() && !op());
#else
#if 1
	{
		static bool loaded = true;
		if (!loaded)
		{
			int width, height, chan;
			auto *buffer = reinterpret_cast<u8vec4 *>(stbi_load("/tmp/frog.png", &width, &height, &chan, 4));

			const auto convert_color = [](u8vec4 v) -> uint16_t {
				unsigned r = v.x >> 3;
				unsigned g = v.y >> 3;
				unsigned b = v.z >> 3;
				unsigned a = v.w >> 7;
				unsigned col = (r << 11) | (g << 6) | (b << 1) | a;
				return col;
			};

			for (unsigned i = 0; i < 2; i++)
			{
				auto *rdram = reinterpret_cast<uint16_t *>(replayers[i]->get_rdram());
				rdram += 1 * 1024 * 1024;
				// Checkerboard
				for (int y = 0; y < height; y++)
					for (int x = 0; x < width; x++)
						rdram[y * 32 + (x ^ 1)] = convert_color(buffer[y * width + x]);
			}

			TileMeta tile_info = {};
			tile_info.stride = 64;
			tile_info.fmt = TextureFormat::RGBA;
			tile_info.size = TextureSize::Bpp16;
			tile_info.mask_s = 5;
			tile_info.mask_t = 5;
			builder.set_tile(0, tile_info);
			builder.set_texture_image(2 * 1024 * 1024, TextureFormat::RGBA, TextureSize::Bpp16, 32);
			builder.load_tile(0, 0, 0, 32, 32);
			builder.set_tile_size_subpixels(0, 3, 3, 128, 128);
			loaded = true;
			stbi_image_free(buffer);
		}
	}
#endif

	static unsigned itr_counter;
	builder.set_viewport({20, 20, 200, 200, 0, 1});
	InputPrimitive prim = {};
	prim.vertices[0].w = 1.0f;
	prim.vertices[1].w = 1.4f;
	prim.vertices[2].w = 1.8f;
	prim.vertices[0].z = 0.0f;
	prim.vertices[1].z = 0.5f;
	prim.vertices[2].z = 1.0f;

	prim.vertices[0].u = 0.0f;
	prim.vertices[0].v = 0.0f;
	prim.vertices[1].u = 50.0f;
	prim.vertices[1].v = 0.0f;
	prim.vertices[2].u = 0.0f;
	prim.vertices[2].v = 50.0f;

	for (unsigned i = 0; i < 3; i++)
	{
		prim.vertices[i].x *= prim.vertices[i].w;
		prim.vertices[i].y *= prim.vertices[i].w;
		prim.vertices[i].z *= prim.vertices[i].w;
	}

	for (unsigned i = 0; i < 3; i++)
		for (unsigned j = 0; j < 3; j++)
			prim.vertices[i].color[j] = 0.0f;
#if 1
	prim.vertices[0].color[0] = 1.0f;
	prim.vertices[1].color[1] = 1.0f;
	prim.vertices[2].color[2] = 1.0f;
#endif

	builder.set_scissor(0, 0, 320, 240);
	//builder.set_scissor(100, 100, 100, 60);

	builder.set_combiner_1cycle({
			                            { RGBMulAdd::Zero,   RGBMulSub::Zero,   RGBMul::Zero,   RGBAdd::Shade },
			                            { AlphaAddSub::Zero, AlphaAddSub::Zero, AlphaMul::Zero, AlphaAddSub::One }
	                            });
	builder.set_depth_write(true);
	builder.set_perspective(true);
	builder.set_dither(RGBDitherMode::Magic);

	do
	{
		switch (itr_counter & 15)
		{
		case 0:
			prim.vertices[0].x = -1.0f * prim.vertices[0].w;
			prim.vertices[0].y = -1.0f * prim.vertices[0].w;
			prim.vertices[1].x = +1.0f * prim.vertices[1].w;
			prim.vertices[1].y = -1.0f * prim.vertices[1].w;
			prim.vertices[2].x = -0.9f * prim.vertices[2].w;
			prim.vertices[2].y = +1.0f * prim.vertices[2].w;
			builder.draw_triangle(prim);
			break;

		case 1:
			//break;
			prim.vertices[0].x = 0.2f;
			prim.vertices[0].y = -0.83f;
			prim.vertices[1].x = -0.1f;
			prim.vertices[1].y = -0.73f;
			prim.vertices[2].x = 0.8f;
			prim.vertices[2].y = -0.63f;
			builder.draw_triangle(prim);
			break;

		case 2:
			break;
			prim.vertices[0].x = 0.8f;
			prim.vertices[0].y = -0.83f + 0.8f;
			prim.vertices[1].x = 0.2f;
			prim.vertices[1].y = -0.73f + 0.8f;
			prim.vertices[2].x = 0.4f;
			prim.vertices[2].y = -0.63f + 0.8f;
			builder.draw_triangle(prim);
			break;

		case 3:
			break;
			prim.vertices[0].x = -0.2f;
			prim.vertices[0].y = -0.83f + 0.8f;
			prim.vertices[1].x = 0.1f;
			prim.vertices[1].y = -0.73f + 0.8f;
			prim.vertices[2].x = -0.8f;
			prim.vertices[2].y = -0.63f + 0.8f;
			builder.draw_triangle(prim);
			break;

		case 4:
			break;
			prim.vertices[0].x = 0.3f;
			prim.vertices[0].y = -0.2f;
			prim.vertices[1].x = 20.0f;
			prim.vertices[1].y = 0.0f;
			prim.vertices[2].x = 0.3f;
			prim.vertices[2].y = 0.2f;
			builder.draw_triangle(prim);
			break;

		case 14:
			builder.end_frame();
			break;

		case 15:
			memset(replayers[0]->get_rdram(), 0, replayers[0]->get_rdram_size());
			memset(replayers[1]->get_rdram(), 0, replayers[1]->get_rdram_size());
			memset(replayers[0]->get_hidden_rdram(), 0, replayers[0]->get_hidden_rdram_size());
			memset(replayers[1]->get_hidden_rdram(), 0, replayers[1]->get_hidden_rdram_size());
			break;

		default:
			break;
		}
		itr_counter++;
	} while (!op());
#endif
}

void DebugApplication::render_frame(double, double)
{
	auto cmd = get_wsi().get_device().request_command_buffer();
	auto rp = get_wsi().get_device().get_swapchain_render_pass(SwapchainRenderPass::Depth);
	cmd->begin_render_pass(rp);

	if (ui.paused && ui.frame_step == 0)
	{
	}
	else if (ui.replay_mode == ReplayMode::VIScanout)
	{
		// Iterate until we get a vdac_scanout event or iterate fails (EOF usually, just freeze on last frame until we rewind.
		unsigned target_frame = ui.replay_vi_frame_count + std::max(ui.frame_step, 1u);
		replay_until([&]() { return ui.replay_vi_frame_count >= target_frame; });
	}
	else if (ui.replay_mode == ReplayMode::DrawCall)
	{
		unsigned target_draw = ui.replay_draw_count + std::max(ui.frame_step, 1u);
		replay_until([&]() { return ui.replay_draw_count >= target_draw; });

		switch (ui.vismode)
		{
		case VisualizationMode::Color:
			update_cached_frame_from_color_pointer(0);
			update_cached_frame_from_color_pointer(1);
			break;

		case VisualizationMode::Depth:
			update_cached_frame_from_depth_pointer(0);
			update_cached_frame_from_depth_pointer(1);
			break;

		case VisualizationMode::Coverage:
			update_cached_frame_from_coverage_pointer(0);
			update_cached_frame_from_coverage_pointer(1);
			break;
		}
	}

	ui.frame_step = 0;

	render_ui(*cmd);
	cmd->end_render_pass();
	get_wsi().get_device().submit(cmd);
}

void DebugApplication::render_ui_vi_scanout(unsigned width, unsigned height)
{
	auto &font = GRANITE_UI_MANAGER()->get_font(UI::FontSize::Large);

	int x = 5, y = 5;
	render_text_top_left_down(font, x, y, "Mode - VI scanout", vec3(1.0f));
	render_text_top_left_down(font, x, y, Util::join("Frames: ", ui.replay_vi_frame_count), vec3(1.0f));

	if (ui.paused)
		render_text_top_left_down(font, x, y, ":: PAUSED ::", vec3(1.0f, 1.0f, 0.0f));
	if (ui.eof)
		render_text_top_left_down(font, x, y, ":: EOF ::", vec3(1.0f, 0.0f, 0.0f));
}

void DebugApplication::render_ui_draw_calls(unsigned width, unsigned height)
{
	auto &large_font = GRANITE_UI_MANAGER()->get_font(UI::FontSize::Large);
	auto &font = GRANITE_UI_MANAGER()->get_font(UI::FontSize::Normal);
	auto &small_font = GRANITE_UI_MANAGER()->get_font(UI::FontSize::Small);

	int x = width - 5;
	int y = 5;

	render_text_top_right_down(large_font, x, y, "Command history", vec3(1.0f));
	for (auto &cmd : ui.command_queue)
	{
		if (command_is_draw_call(cmd))
			render_text_top_right_down(font, x, y, command_name(cmd), vec3(1.0f));
		else
			render_text_top_right_down(small_font, x, y, command_name(cmd), vec3(1.0f));
	}
}

void DebugApplication::render_ui_draw_call(unsigned width, unsigned height)
{
	auto &font = GRANITE_UI_MANAGER()->get_font(UI::FontSize::Large);
	int x = 5, y = 5;

	switch (ui.vismode)
	{
	case VisualizationMode::Depth:
		render_text_top_left_down(font, x, y, "Mode - Draw Call - Depth", vec3(1.0f));
		break;

	case VisualizationMode::Color:
		render_text_top_left_down(font, x, y, "Mode - Draw Call - Color", vec3(1.0f));
		break;

	case VisualizationMode::Coverage:
		render_text_top_left_down(font, x, y, "Mode - Draw Call - Coverage", vec3(1.0f));
		break;
	}

	render_text_top_left_down(font, x, y, Util::join("Frames: ", ui.replay_vi_frame_count, " Draws: ", ui.replay_draw_count_in_frame), vec3(1.0f));

	if (ui.paused)
		render_text_top_left_down(font, x, y, ":: PAUSED ::", vec3(1.0f, 1.0f, 0.0f));
	if (ui.eof)
		render_text_top_left_down(font, x, y, ":: EOF ::", vec3(1.0f, 0.0f, 0.0f));

	render_ui_draw_calls(width, height);
}

void DebugApplication::render_text_top_left_down(const Font &font, int &x, int &y, const std::string &text, const vec3 &color)
{
	vec2 geometry = font.get_text_geometry(text.c_str());
	geometry += 12.0f;
	y += 2;
	ui.flat_renderer.render_quad(vec3(x, y, 1.0f), geometry, vec4(0.0f, 0.0f, 0.0f, 0.9f));
	ui.flat_renderer.render_text(font, text.c_str(), vec3(x, y, 0.0f), geometry, vec4(color, 1.0f), Font::Alignment::Center);
	y += int(geometry.y);
}

void DebugApplication::render_text_top_right_down(const Font &font, int &x, int &y, const std::string &text, const vec3 &color)
{
	vec2 geometry = font.get_text_geometry(text.c_str());
	geometry += 12.0f;
	y += 2;
	ui.flat_renderer.render_quad(vec3(x - geometry.x, y, 1.0f), geometry, vec4(0.0f, 0.0f, 0.0f, 0.9f));
	ui.flat_renderer.render_text(font, text.c_str(), vec3(x - geometry.x, y, 0.0f), geometry, vec4(color, 1.0f), Font::Alignment::Center);
	y += int(geometry.y);
}

void DebugApplication::render_text_bottom_right_up(const Font &font, int &x, int &y, const std::string &text, const vec3 &color)
{
	vec2 geometry = font.get_text_geometry(text.c_str());
	geometry += 12.0f;
	y -= 2;
	y -= int(geometry.y);
	ui.flat_renderer.render_quad(vec3(float(x) - geometry.x, y, 1.0f), geometry, vec4(0.0f, 0.0f, 0.0f, 0.9f));
	ui.flat_renderer.render_text(font, text.c_str(), vec3(float(x) - geometry.x, y, 0.0f), geometry, vec4(color, 1.0f), Font::Alignment::Center);
}

void DebugApplication::render_text_bottom_left_up(const Font &font, int &x, int &y, const std::string &text, const vec3 &color)
{
	vec2 geometry = font.get_text_geometry(text.c_str());
	geometry += 12.0f;
	y -= 2;
	y -= int(geometry.y);
	ui.flat_renderer.render_quad(vec3(float(x), y, 1.0f), geometry, vec4(0.0f, 0.0f, 0.0f, 0.9f));
	ui.flat_renderer.render_text(font, text.c_str(), vec3(float(x), y, 0.0f), geometry, vec4(color, 1.0f), Font::Alignment::Center);
}

static vec3 message_type_to_color(MessageType type)
{
	switch (type)
	{
	case MessageType::Info:
		return vec3(1.0f);

	case MessageType::Warn:
		return vec3(0.8f, 1.0f, 0.0f);

	case MessageType::Error:
		return vec3(1.0f, 0.2f, 0.2f);

	default:
		return vec3(0.0f);
	}
}

void DebugApplication::render_ui_messages(unsigned width, unsigned height)
{
	auto &font = GRANITE_UI_MANAGER()->get_font(UI::FontSize::Normal);
	int x = int(width) - 5, y = int(height) - 5;

	unsigned message_count = 0;
	for (auto &message : ui.current_messages)
	{
		if (message.current_lifetime > 0)
		{
			message.current_lifetime--;
			render_text_bottom_right_up(font, x, y, message.message, message_type_to_color(message.type));
			if (++message_count == 4)
				break;
		}
	}

	auto itr = std::remove_if(ui.current_messages.begin(), ui.current_messages.end(), [](const UIMessage &msg) {
		return msg.current_lifetime == 0;
	});
	ui.current_messages.erase(itr, ui.current_messages.end());
}

void DebugApplication::render_ui_view_state(unsigned width, unsigned height)
{
	auto &font = GRANITE_UI_MANAGER()->get_font(UI::FontSize::Large);
	int x = 5, y = int(height) - 5;
	auto rect = get_texture_rect();
	auto msg = Util::join("View: ", "[(", rect.offset.x, ", ", rect.offset.y, ")", ", (", rect.size.x, ", ", rect.size.y, ")]");
	render_text_bottom_left_up(font, x, y, msg, vec3(1.0f));

	if (cached_frame[0].width && cached_frame[0].height)
	{
		float tex_x = rect.offset.x + rect.size.x * muglm::fract(2.0f * (view.last_mouse_x + 0.5f) / float(view.window_width));
		float tex_y = rect.offset.y + rect.size.y * ((view.last_mouse_y + 0.5f) / float(view.window_height));
		int itex_x = clamp(int(tex_x), 0, int(cached_frame[0].width) - 1);
		int itex_y = clamp(int(tex_y), 0, int(cached_frame[0].height) - 1);
		unsigned pixel = itex_y * cached_frame[0].width + itex_x;

		const auto &pix = cached_frame[0].buffer[pixel];
		const auto *pix_other = cached_frame[1].buffer.empty() ? nullptr : &cached_frame[1].buffer[pixel];

		auto pixel_msg = Util::join("Hover: [(", itex_x, ", ", itex_y, ")]");
		render_text_bottom_left_up(font, x, y, pixel_msg, vec3(1.0f));

		if (ui.replay_mode == ReplayMode::VIScanout)
		{
			pixel_msg = Util::join("RGB (8-bit): [(", int(pix.x), ", ", int(pix.y), ", ", int(pix.z), ")]");
			if (pix_other)
				pixel_msg += Util::join(" [(", int(pix_other->x), ", ", int(pix_other->y), ", ", int(pix_other->z), ")]");
			render_text_bottom_left_up(font, x, y, pixel_msg, vec3(1.0f));
		}
		else if (ui.replay_mode == ReplayMode::DrawCall && cached_color_image.fb_size == 2 && ui.vismode == VisualizationMode::Color)
		{
			pixel_msg = Util::join("RGB (5-bit): [(", int(pix.x >> 3), ", ", int(pix.y >> 3), ", ", int(pix.z >> 3), ")]");
			if (pix_other)
				pixel_msg += Util::join(" [(", int(pix_other->x >> 3), ", ", int(pix_other->y >> 3), ", ", int(pix_other->z >> 3), ")]");
			render_text_bottom_left_up(font, x, y, pixel_msg, vec3(1.0f));
		}
		else if (ui.replay_mode == ReplayMode::DrawCall && ui.vismode == VisualizationMode::Depth)
		{
			unsigned color =
					((pix.x >> 3) << 11) |
					((pix.y >> 3) << 6) |
					((pix.z >> 3) << 1);
			unsigned dz = pix.w;
			color >>= 2;

			pixel_msg = Util::join("Depth (16-bit): [", color, " (", dz, ")]");
			if (pix_other)
			{
				color = ((pix_other->x >> 3) << 11) |
				        ((pix_other->y >> 3) << 6) |
				        ((pix_other->z >> 3) << 1);
				dz = pix_other->w;
				color >>= 2;
				pixel_msg += Util::join(" [", color, " (", dz, ")]");
			}
			render_text_bottom_left_up(font, x, y, pixel_msg, vec3(1.0f));
		}
		else if (ui.replay_mode == ReplayMode::DrawCall && ui.vismode == VisualizationMode::Coverage)
		{
			unsigned cov = pix.x >> 5;
			pixel_msg = Util::join("Coverage (3-bit): [", cov, "]");
			if (pix_other)
			{
				cov = (pix_other->x >> 5);
				pixel_msg += Util::join(" [", cov, "]");
			}
			render_text_bottom_left_up(font, x, y, pixel_msg, vec3(1.0f));
		}
	}
}

void DebugApplication::render_ui(CommandBuffer &cmd)
{
	ui.flat_renderer.begin();
	if (ui.replay_mode == ReplayMode::VIScanout)
	{
		render_ui_vi_scanout(unsigned(cmd.get_viewport().width), unsigned(cmd.get_viewport().height));
	}
	else if (ui.replay_mode == ReplayMode::DrawCall)
	{
		render_ui_draw_call(unsigned(cmd.get_viewport().width), unsigned(cmd.get_viewport().height));
	}

	render_ui_view_state(unsigned(cmd.get_viewport().width), unsigned(cmd.get_viewport().height));
	render_ui_messages(unsigned(cmd.get_viewport().width), unsigned(cmd.get_viewport().height));
	render_scanout_texture(cmd);
	ui.flat_renderer.flush(cmd, vec3(0.0f), vec3(cmd.get_viewport().width, cmd.get_viewport().height, float(0xffff)));
}

void DebugApplication::add_message(std::string message, MessageType type)
{
	UIMessage msg = { std::move(message), type, 80, 80 };
	ui.current_messages.push_back(std::move(msg));
}

namespace Granite
{
Application *application_create(int argc, char **argv)
{
	application_dummy();
	if (argc != 2)
		return nullptr;
	std::string path = argv[1];
	return new DebugApplication(path);
}
}
