/* Copyright (c) 2022 Themaister
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

#include "rdp_device.hpp"
#include "context.hpp"
#include "device.hpp"
#include "wsi.hpp"
#include "GLFW/glfw3.h"
#include "logging.hpp"
#include <stddef.h>
#include <cmath>

// The non-standalone parallel-rdp requires some filesystem access for shaders which we have to handle here.
// When using the standalone build, all shaders are precompiled and codepath that is ifdeffed here can be omitted.
#define PARALLEL_RDP_STANDALONE 0

#if !PARALLEL_RDP_STANDALONE
#include "global_managers_init.hpp"
#endif

// A demo showing how to blit parallel-rdp frames on screen using Granite's WSI system.
// GLFW is used here as the window, replace that with whatever you have.

constexpr unsigned WIDTH = 1280;
constexpr unsigned HEIGHT = 720;

static void fb_size_cb(GLFWwindow *window, int width, int height);

// Basic WSI platform implementation based on GLFW here since Granite already integrates it.
// Replace with your own as necessary.

struct GLFWPlatform : public Vulkan::WSIPlatform
{
	VkSurfaceKHR create_surface(VkInstance instance, VkPhysicalDevice) override
	{
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
			return VK_NULL_HANDLE;

		update_framebuffer_size();
		return surface;
	}

	std::vector<const char *> get_instance_extensions() override
	{
		// Here we only expect VK_KHR_surface and the platform surface extension.
		uint32_t count;
		const char **ext = glfwGetRequiredInstanceExtensions(&count);
		return { ext, ext + count };
	}

	uint32_t get_surface_width() override
	{
		return width;
	}

	uint32_t get_surface_height() override
	{
		return height;
	}

	const VkApplicationInfo *get_application_info() override
	{
		// We require at least Vulkan 1.1.
		static const VkApplicationInfo info = { VK_STRUCTURE_TYPE_APPLICATION_INFO,
		                                        nullptr,
		                                        "parallel-rdp-test", 0,
		                                        "parallel-rdp", 0,
		                                        VK_API_VERSION_1_1 };
		return &info;
	}

	bool alive(Vulkan::WSI & /*wsi*/) override
	{
		return !glfwWindowShouldClose(window);
	}

	void poll_input() override
	{
		glfwPollEvents();
	}

	void update_framebuffer_size()
	{
		int actual_width, actual_height;
		glfwGetFramebufferSize(window, &actual_width, &actual_height);
		width = unsigned(actual_width);
		height = unsigned(actual_height);
	}

	void set_window(GLFWwindow *window_)
	{
		window = window_;
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, fb_size_cb);
		update_framebuffer_size();
	}

	void notify_resize(unsigned width_, unsigned height_)
	{
		// Technically, we don't really have to do this.
		// We can rely on OUT_OF_DATE error to work,
		// but the resizing experience is smoother this way.
		resize = true;
		width = width_;
		height = height_;
	}

	unsigned width = 0;
	unsigned height = 0;
	GLFWwindow *window = nullptr;
};

static void fb_size_cb(GLFWwindow *window, int width, int height)
{
	auto *glfw = static_cast<GLFWPlatform *>(glfwGetWindowUserPointer(window));
	VK_ASSERT(width != 0 && height != 0);
	glfw->notify_resize(width, height);
}

constexpr unsigned SCANOUT_ORIGIN = 1024;
constexpr unsigned SCANOUT_WIDTH = 320;
constexpr unsigned SCANOUT_HEIGHT = 240;

static void setup_default_vi_registers(RDP::CommandProcessor &processor)
{
	processor.set_vi_register(RDP::VIRegister::Control,
	                          RDP::VI_CONTROL_AA_MODE_RESAMP_REPLICATE_BIT |
	                          RDP::VI_CONTROL_TYPE_RGBA8888_BIT);
	processor.set_vi_register(RDP::VIRegister::Origin, SCANOUT_ORIGIN);
	processor.set_vi_register(RDP::VIRegister::Width, SCANOUT_WIDTH);
	processor.set_vi_register(RDP::VIRegister::VSync, RDP::VI_V_SYNC_NTSC);
	processor.set_vi_register(RDP::VIRegister::XScale,
	                          RDP::make_vi_scale_register(512, 0));
	processor.set_vi_register(RDP::VIRegister::YScale,
	                          RDP::make_vi_scale_register(1024, 0));
	processor.set_vi_register(RDP::VIRegister::VStart,
	                          RDP::make_vi_start_register(RDP::VI_V_OFFSET_NTSC,
	                                                      RDP::VI_V_OFFSET_NTSC + 224 * 2));
	processor.set_vi_register(RDP::VIRegister::HStart,
	                          RDP::make_vi_start_register(RDP::VI_H_OFFSET_NTSC,
	                                                      RDP::VI_H_OFFSET_NTSC + 640));
}

static void update_vram(void *vram_ptr, unsigned frame_index)
{
	auto *base = static_cast<uint32_t *>(vram_ptr) + SCANOUT_ORIGIN / 4;

	for (unsigned y = 0; y < SCANOUT_HEIGHT; y++)
	{
		for (unsigned x = 0; x < SCANOUT_WIDTH; x++)
		{
			// Generate a funky pattern.

			float r = std::sin(float(x) * 0.134f + float(y) * 0.234f + float(frame_index) * 0.05f);
			float g = std::sin(float(x) * 0.434f + float(y) * 0.234f + float(frame_index) * 0.02f);
			float b = std::sin(float(x) * -0.234f + float(y) * -0.234f + float(frame_index) * 0.03f);

			r = r * 0.4f + 0.5f;
			g = g * 0.4f + 0.5f;
			b = b * 0.4f + 0.5f;

			auto ur = unsigned(r * 255.0f);
			auto ug = unsigned(g * 255.0f);
			auto ub = unsigned(b * 255.0f);

			uint32_t color = (ur << 24) | (ug << 16) | (ub << 8);
			base[y * SCANOUT_WIDTH + x] = color;
		}
	}
}

// Normally we use the shader manager in Granite or slangmosh to do this gracefully, but for
// just blitting a quad in integration code without the full Granite build
// we can just feed it raw SPIR-V.

// Output generated from shaderc with:
// glslc -Os -mfmt=c -o /tmp/vert.h /tmp/test.vert
// glslc -Os -mfmt=c -o /tmp/frag.h /tmp/test.frag

// Vulkan GLSL code:

#if 0
#version 450

layout(location = 0) out vec2 vUV;

void main()
{
	if (gl_VertexIndex == 0)
	{
		gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
	}
	else if (gl_VertexIndex == 1)
	{
		gl_Position = vec4(-1.0, +3.0, 0.0, 1.0);
	}
	else
	{
		gl_Position = vec4(+3.0, -1.0, 0.0, 1.0);
	}

	vUV = gl_Position.xy * 0.5 + 0.5;
}
#endif

#if 0
#version 450

layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 0) uniform sampler2D uTexture;
layout(location = 0) in vec2 vUV;

void main()
{
	FragColor = textureLod(uTexture, vUV, 0.0);
}
#endif

static const uint32_t vertex_spirv[] =
		{0x07230203,0x00010000,0x000d000a,0x00000034,
		 0x00000000,0x00020011,0x00000001,0x0006000b,
		 0x00000001,0x4c534c47,0x6474732e,0x3035342e,
		 0x00000000,0x0003000e,0x00000000,0x00000001,
		 0x0008000f,0x00000000,0x00000004,0x6e69616d,
		 0x00000000,0x00000008,0x00000016,0x0000002b,
		 0x00040047,0x00000008,0x0000000b,0x0000002a,
		 0x00050048,0x00000014,0x00000000,0x0000000b,
		 0x00000000,0x00050048,0x00000014,0x00000001,
		 0x0000000b,0x00000001,0x00050048,0x00000014,
		 0x00000002,0x0000000b,0x00000003,0x00050048,
		 0x00000014,0x00000003,0x0000000b,0x00000004,
		 0x00030047,0x00000014,0x00000002,0x00040047,
		 0x0000002b,0x0000001e,0x00000000,0x00020013,
		 0x00000002,0x00030021,0x00000003,0x00000002,
		 0x00040015,0x00000006,0x00000020,0x00000001,
		 0x00040020,0x00000007,0x00000001,0x00000006,
		 0x0004003b,0x00000007,0x00000008,0x00000001,
		 0x0004002b,0x00000006,0x0000000a,0x00000000,
		 0x00020014,0x0000000b,0x00030016,0x0000000f,
		 0x00000020,0x00040017,0x00000010,0x0000000f,
		 0x00000004,0x00040015,0x00000011,0x00000020,
		 0x00000000,0x0004002b,0x00000011,0x00000012,
		 0x00000001,0x0004001c,0x00000013,0x0000000f,
		 0x00000012,0x0006001e,0x00000014,0x00000010,
		 0x0000000f,0x00000013,0x00000013,0x00040020,
		 0x00000015,0x00000003,0x00000014,0x0004003b,
		 0x00000015,0x00000016,0x00000003,0x0004002b,
		 0x0000000f,0x00000017,0xbf800000,0x0004002b,
		 0x0000000f,0x00000018,0x00000000,0x0004002b,
		 0x0000000f,0x00000019,0x3f800000,0x0007002c,
		 0x00000010,0x0000001a,0x00000017,0x00000017,
		 0x00000018,0x00000019,0x00040020,0x0000001b,
		 0x00000003,0x00000010,0x0004002b,0x00000006,
		 0x0000001f,0x00000001,0x0004002b,0x0000000f,
		 0x00000023,0x40400000,0x0007002c,0x00000010,
		 0x00000024,0x00000017,0x00000023,0x00000018,
		 0x00000019,0x0007002c,0x00000010,0x00000027,
		 0x00000023,0x00000017,0x00000018,0x00000019,
		 0x00040017,0x00000029,0x0000000f,0x00000002,
		 0x00040020,0x0000002a,0x00000003,0x00000029,
		 0x0004003b,0x0000002a,0x0000002b,0x00000003,
		 0x0004002b,0x0000000f,0x0000002f,0x3f000000,
		 0x0005002c,0x00000029,0x00000033,0x0000002f,
		 0x0000002f,0x00050036,0x00000002,0x00000004,
		 0x00000000,0x00000003,0x000200f8,0x00000005,
		 0x0004003d,0x00000006,0x00000009,0x00000008,
		 0x000500aa,0x0000000b,0x0000000c,0x00000009,
		 0x0000000a,0x000300f7,0x0000000e,0x00000000,
		 0x000400fa,0x0000000c,0x0000000d,0x0000001d,
		 0x000200f8,0x0000000d,0x00050041,0x0000001b,
		 0x0000001c,0x00000016,0x0000000a,0x0003003e,
		 0x0000001c,0x0000001a,0x000200f9,0x0000000e,
		 0x000200f8,0x0000001d,0x000500aa,0x0000000b,
		 0x00000020,0x00000009,0x0000001f,0x000300f7,
		 0x00000022,0x00000000,0x000400fa,0x00000020,
		 0x00000021,0x00000026,0x000200f8,0x00000021,
		 0x00050041,0x0000001b,0x00000025,0x00000016,
		 0x0000000a,0x0003003e,0x00000025,0x00000024,
		 0x000200f9,0x00000022,0x000200f8,0x00000026,
		 0x00050041,0x0000001b,0x00000028,0x00000016,
		 0x0000000a,0x0003003e,0x00000028,0x00000027,
		 0x000200f9,0x00000022,0x000200f8,0x00000022,
		 0x000200f9,0x0000000e,0x000200f8,0x0000000e,
		 0x00050041,0x0000001b,0x0000002c,0x00000016,
		 0x0000000a,0x0004003d,0x00000010,0x0000002d,
		 0x0000002c,0x0007004f,0x00000029,0x0000002e,
		 0x0000002d,0x0000002d,0x00000000,0x00000001,
		 0x0005008e,0x00000029,0x00000030,0x0000002e,
		 0x0000002f,0x00050081,0x00000029,0x00000032,
		 0x00000030,0x00000033,0x0003003e,0x0000002b,
		 0x00000032,0x000100fd,0x00010038};

static const uint32_t fragment_spirv[] =
		{0x07230203,0x00010000,0x000d000a,0x00000015,
		 0x00000000,0x00020011,0x00000001,0x0006000b,
		 0x00000001,0x4c534c47,0x6474732e,0x3035342e,
		 0x00000000,0x0003000e,0x00000000,0x00000001,
		 0x0007000f,0x00000004,0x00000004,0x6e69616d,
		 0x00000000,0x00000009,0x00000011,0x00030010,
		 0x00000004,0x00000007,0x00040047,0x00000009,
		 0x0000001e,0x00000000,0x00040047,0x0000000d,
		 0x00000022,0x00000000,0x00040047,0x0000000d,
		 0x00000021,0x00000000,0x00040047,0x00000011,
		 0x0000001e,0x00000000,0x00020013,0x00000002,
		 0x00030021,0x00000003,0x00000002,0x00030016,
		 0x00000006,0x00000020,0x00040017,0x00000007,
		 0x00000006,0x00000004,0x00040020,0x00000008,
		 0x00000003,0x00000007,0x0004003b,0x00000008,
		 0x00000009,0x00000003,0x00090019,0x0000000a,
		 0x00000006,0x00000001,0x00000000,0x00000000,
		 0x00000000,0x00000001,0x00000000,0x0003001b,
		 0x0000000b,0x0000000a,0x00040020,0x0000000c,
		 0x00000000,0x0000000b,0x0004003b,0x0000000c,
		 0x0000000d,0x00000000,0x00040017,0x0000000f,
		 0x00000006,0x00000002,0x00040020,0x00000010,
		 0x00000001,0x0000000f,0x0004003b,0x00000010,
		 0x00000011,0x00000001,0x0004002b,0x00000006,
		 0x00000013,0x00000000,0x00050036,0x00000002,
		 0x00000004,0x00000000,0x00000003,0x000200f8,
		 0x00000005,0x0004003d,0x0000000b,0x0000000e,
		 0x0000000d,0x0004003d,0x0000000f,0x00000012,
		 0x00000011,0x00070058,0x00000007,0x00000014,
		 0x0000000e,0x00000012,0x00000002,0x00000013,
		 0x0003003e,0x00000009,0x00000014,0x000100fd,
		 0x00010038};

static void render_frame(Vulkan::Device &device, RDP::CommandProcessor &processor)
{
	RDP::ScanoutOptions options = {};
	Vulkan::ImageHandle image = processor.scanout(options);

	// Normally reflection is automated.
	Vulkan::ResourceLayout vertex_layout = {};
	Vulkan::ResourceLayout fragment_layout = {};
	fragment_layout.output_mask = 1 << 0;
	fragment_layout.sets[0].sampled_image_mask = 1 << 0;

	// This request is cached.
	auto *program = device.request_program(vertex_spirv, sizeof(vertex_spirv),
	                                       fragment_spirv, sizeof(fragment_spirv),
	                                       &vertex_layout,
	                                       &fragment_layout);

	// Blit image on screen.
	auto cmd = device.request_command_buffer();
	{
		auto rp = device.get_swapchain_render_pass(Vulkan::SwapchainRenderPass::ColorOnly);
		cmd->begin_render_pass(rp);

		VkViewport vp = cmd->get_viewport();
		// Adjust the viewport here for aspect ratio correction.

		cmd->set_program(program);

		// Basic default render state.
		cmd->set_opaque_state();
		cmd->set_depth_test(false, false);
		cmd->set_cull_mode(VK_CULL_MODE_NONE);

		cmd->set_texture(0, 0, image->get_view(), Vulkan::StockSampler::LinearClamp);
		cmd->set_viewport(vp);

		// The vertices are constants in the shader.
		// Draws fullscreen quad using oversized triangle.
		cmd->draw(3);

		cmd->end_render_pass();
	}
	device.submit(cmd);
}

int main()
{
	if (!glfwInit())
	{
		LOGE("Failed to initialize GLFW.\n");
		return EXIT_FAILURE;
	}

	if (!Vulkan::Context::init_loader(nullptr))
	{
		LOGE("Failed to initialize Vulkan loader.\n");
		return EXIT_FAILURE;
	}

	GLFWPlatform platform;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto *window = glfwCreateWindow(WIDTH, HEIGHT, "parallel-rdp-demo",
	                                nullptr, nullptr);
	platform.set_window(window);
	glfwShowWindow(window);

	Vulkan::Context::SystemHandles handles = {};
	// For Granite to have access to filesystem, threading, etc, we need to
	// pass down handles.
	// For this kind of integration where we only need to blit stuff,
	// we don't need any of this.
	// We don't have to build in lots of code for shader compilers, filesystems, etc ...

#if !PARALLEL_RDP_STANDALONE
	// However, this only applies to the standalone parallel-rdp build in
	// Themaister/parallel-rdp-standalone.
	// This repo is used for development, and does runtime shader compilation, so make sure we enable filesystem
	// support if we have to.
	Granite::Global::init(Granite::Global::MANAGER_FEATURE_FILESYSTEM_BIT);
	handles.filesystem = GRANITE_FILESYSTEM();
#endif

	Vulkan::WSI wsi;
	wsi.set_platform(&platform);

	// By default, the backbuffer is sRGB, which is likely not what we want,
	// since the parallel-rdp scanout is UNORM.
	wsi.set_backbuffer_srgb(false);

	// See wsi.hpp for how to use custom VkInstance, VkDevice, etc.
	// For this test we will use the simple API.
	if (!wsi.init_simple(1, handles))
	{
		LOGE("Failed to initialize WSI.\n");
		return EXIT_FAILURE;
	}

	auto &device = wsi.get_device();

	constexpr unsigned RDRAM_SIZE = 4 * 1024 * 1024;

	// The imported base pointer must be sufficiently aligned to
	// support external host memory. 64 KiB is good enough.
	// rdram_offset can be used to point to an offset within rdram_ptr.
	void *rdram_ptr = Util::memalign_calloc(64 * 1024, RDRAM_SIZE);
	auto processor = std::make_unique<RDP::CommandProcessor>(
			device,
			rdram_ptr, 0 /* offset */,
			RDRAM_SIZE, RDRAM_SIZE / 8,
			RDP::CommandProcessorFlags{});

	if (!processor->device_is_supported())
	{
		LOGE("Vulkan device does not support required features for parallel-rdp.\n");
		return EXIT_FAILURE;
	}

	setup_default_vi_registers(*processor);
	unsigned frame_count = 0;

	while (!glfwWindowShouldClose(window))
	{
		// Begins a new frame context and calls vkAcquireNextImageKHR.
		// For frame pacing and latency reasons,
		// it's important that we're inside a WSI frame context at all times since
		// begin_frame() is a blocking call.
		// WSI will call poll_input() callback after the blocking parts are done to reduce input latency.
		// If the abstraction only supports a notion of "SwapBuffers",
		// begin_frame() should be called *before* returning back to application, e.g. swap_buffers() would look like:
		//  - end_frame()
		//  - begin_frame()
		// Application can then ensure first and last frames are paired up with appropriate begin/end calls.
		wsi.begin_frame();

		// Update VRAM with something that animates for purposes of scanout.
		update_vram(rdram_ptr, frame_count++);

		// Scanout VI and blit the result to screen.
		render_frame(device, *processor);

		// Flushes work and calls vkQueuePresentKHR.
		wsi.end_frame();

		// Optional optimization. If the device is otherwise not being concurrently accessed by other threads
		// we can promote caches to read-only to avoid some locks in subsequent frames.
		device.promote_read_write_caches_to_read_only();
	}

	// Make sure WSI is destroyed or torn down before we destroy GLFW window.
	processor.reset();
	wsi.teardown();

	glfwDestroyWindow(window);
	glfwTerminate();
	Util::memalign_free(rdram_ptr);
}
