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

#pragma once

#include <random>
#include "logging.hpp"
#include "context.hpp"
#include "device.hpp"
#include "rdp_command_builder.hpp"
#include "replayer_driver.hpp"
#include "os_filesystem.hpp"
#include "global_managers.hpp"
#include "thread_group.hpp"
#ifdef ANDROID
#include "android.hpp"
#endif
#include <string.h>
#define LOG_FAILURE() LOGE("Failed at %s:%d.\n", __FILE__, __LINE__)

namespace RDP
{
struct RNG
{
	std::mt19937 rnd {1337};
	std::uniform_real_distribution<float> udist = std::uniform_real_distribution<float>(0.0, 1.0);

	inline float generate(float lo, float hi)
	{
		return udist(rnd) * (hi - lo) + lo;
	}

	inline bool boolean()
	{
		return (rnd() & 1) != 0;
	}
};

struct Interface : ReplayerEventInterface
{
	inline void update_screen(const void *data, unsigned width, unsigned height, unsigned pitch) override;
	inline void notify_command(Op cmd_id, uint32_t num_words, const uint32_t *words) override;
	inline void message(MessageType, const char *) override {}
	inline void eof() override { is_eof = true; }
	inline void set_context_index(unsigned index) override;
	inline void signal_complete() override;

	unsigned draw_calls_for_context[2] = {};
	unsigned frame_count_for_context[2] = {};
	unsigned syncs_for_context[2] = {};
	unsigned current_context = 0;

	struct RGBA { uint8_t r, g, b, a; };
	std::vector<RGBA> scanout_result[2];
	unsigned widths[2] = {};
	unsigned heights[2] = {};

	bool is_eof = false;

	struct
	{
		uint32_t addr = 0;
		uint32_t size = 0;
		uint32_t width = 0;
		uint32_t depth_addr = 0;
	} fb;
};

void Interface::update_screen(const void *data, unsigned width, unsigned height, unsigned pitch)
{
	scanout_result[current_context].resize(width * height);
	for (unsigned y = 0; y < height; y++)
	{
		memcpy(scanout_result[current_context].data() + y * width,
		       static_cast<const uint32_t *>(data) + y * pitch,
		       width * sizeof(RGBA));
	}
	widths[current_context] = width;
	heights[current_context] = height;
	frame_count_for_context[current_context]++;
	draw_calls_for_context[current_context] = 0;
	syncs_for_context[current_context] = 0;
}

void Interface::set_context_index(unsigned index)
{
	current_context = index;
}

void Interface::notify_command(Op cmd_id, uint32_t, const uint32_t *words)
{
	if (command_is_draw_call(cmd_id))
	{
		draw_calls_for_context[current_context]++;
	}
	else if (cmd_id == Op::SetColorImage)
	{
		fb.size = (words[0] >> 19) & 3;
		fb.addr = words[1] & 0xffffff;
		fb.width = (words[0] & 1023) + 1;
	}
	else if (cmd_id == Op::SetMaskImage)
	{
		fb.depth_addr = words[1] & 0xffffff;
	}
}

void Interface::signal_complete()
{
	syncs_for_context[current_context]++;
}

struct ReplayerState
{
	~ReplayerState()
	{
		// Ensure that debug callbacks are flushed.
		device->wait_idle();
	}

	inline bool init();
	inline bool init(Vulkan::Device *device);
	inline bool init(DumpPlayer &dump);
	Vulkan::Context context;
	std::unique_ptr<Vulkan::Device> owned_device;
	Vulkan::Device *device = nullptr;
	std::unique_ptr<ReplayerDriver> reference, gpu, gpu_scaled;
	std::unique_ptr<ReplayerDriver> combined;
	CommandBuilder builder;
	Interface iface;

	inline bool init_common(Vulkan::Device *custom_device = nullptr);
};

bool ReplayerState::init_common(Vulkan::Device *custom_device)
{
	if (custom_device)
	{
		device = custom_device;
	}
	else
	{
		owned_device.reset(new Vulkan::Device);
		device = owned_device.get();

		if (!Vulkan::Context::init_loader(nullptr))
		{
			LOGE("Failed to init Vulkan loader.\n");
			return false;
		}

		Vulkan::Context::SystemHandles handles;
		handles.filesystem = GRANITE_FILESYSTEM();
		handles.thread_group = GRANITE_THREAD_GROUP();
		handles.timeline_trace_file = handles.thread_group->get_timeline_trace_file();
		context.set_system_handles(handles);

		if (!context.init_instance_and_device(nullptr, 0, nullptr, 0, Vulkan::CONTEXT_CREATION_DISABLE_BINDLESS_BIT))
		{
			LOGE("Failed to create Vulkan context.\n");
			return false;
		}
		device->set_context(context);
	}

	return true;
}

bool ReplayerState::init()
{
	return init(nullptr);
}

bool ReplayerState::init(Vulkan::Device *device_)
{
	if (!init_common(device_))
		return false;

	reference = create_replayer_driver_angrylion(builder, iface);
	gpu = create_replayer_driver_parallel(*device, builder, iface, device_ != nullptr);
	gpu_scaled = create_replayer_driver_parallel(*device, builder, iface, device_ != nullptr, true);
	combined = create_side_by_side_driver(reference.get(), gpu.get(), iface);
	builder.set_command_interface(combined.get());
	return true;
}

bool ReplayerState::init(DumpPlayer &dump)
{
	if (!init_common())
		return false;

	reference = create_replayer_driver_angrylion(dump, iface);
	gpu = create_replayer_driver_parallel(*device, dump, iface);
	combined = create_side_by_side_driver(reference.get(), gpu.get(), iface);
	dump.set_command_interface(combined.get());
	return true;
}

static inline bool compare_memory(const char *tag, const uint8_t *reference_, const uint8_t *gpu_, size_t size,
                                  uint32_t *fault_addr)
{
	if (!memcmp(reference_, gpu_, size))
	{
		auto *reference = reinterpret_cast<const uint32_t *>(reference_);
		size /= sizeof(uint32_t);

		bool nonzero = false;
		for (size_t i = 0; i < size; i++)
		{
			if (reference[i] != 0)
			{
				nonzero = true;
				break;
			}
		}
		if (!nonzero)
			LOGW("RDRAM is completely zero, might not be a valuable test.\n");
		return true;
	}
	else
	{
		auto *reference = reference_;
		auto *reference16 = reinterpret_cast<const uint16_t *>(reference_);
		auto *reference32 = reinterpret_cast<const uint32_t *>(reference_);
		auto *gpu = gpu_;
		auto *gpu16 = reinterpret_cast<const uint16_t *>(gpu_);
		auto *gpu32 = reinterpret_cast<const uint32_t *>(gpu_);

		bool nonzero = false;
		for (size_t i = 0; i < size; i++)
		{
			if (reference[i ^ 3] != gpu[i ^ 3])
			{
				LOGE("  8-bit coord: (%d, %d)\n", int(i % 320), int(i / 320));
				LOGE("Memory delta found at byte %zu for %s, (ref) 0x%02x != (gpu) 0x%02x!\n", i, tag, reference[i ^ 3],
				     gpu[i ^ 3]);

				LOGE("  16-bit coord: (%d, %d)\n", int((i >> 1) % 320), int((i >> 1) / 320));
				LOGE("Memory delta found at word %zu for %s, (ref) 0x%02x != (gpu) 0x%02x!\n", i >> 1, tag, reference16[(i >> 1) ^ 1],
				     gpu16[(i >> 1) ^ 1]);

				LOGE("  32-bit coord: (%d, %d)\n", int((i >> 2) % 320), int((i >> 2) / 320));
				LOGE("Memory delta found at dword %zu for %s, (ref) 0x%02x != (gpu) 0x%02x!\n", i >> 2, tag, reference32[i >> 2],
				     gpu32[i >> 2]);

				if (fault_addr)
					*fault_addr = i;
				return false;
			}

			if (reference[i ^ 3] != 0)
				nonzero = true;
		}

		if (!nonzero)
			LOGW("RDRAM is completely zero, might not be a valuable test.\n");
	}

	return true;
}

static inline bool compare_rdram(ReplayerDriver &reference, ReplayerDriver &gpu,
                                 uint32_t *fault_addr = nullptr, bool *fault_hidden = nullptr)
{
	auto *rdram_reference = reference.get_rdram();
	auto *rdram_gpu = gpu.get_rdram();
	if (!compare_memory("RDRAM", rdram_reference, rdram_gpu, gpu.get_rdram_size(), fault_addr))
	{
		if (fault_hidden)
			*fault_hidden = false;
		return false;
	}

	auto *hidden_reference = reference.get_hidden_rdram();
	auto *hidden_gpu = gpu.get_hidden_rdram();
	if (!compare_memory("Hidden RDRAM", hidden_reference, hidden_gpu, gpu.get_hidden_rdram_size(), fault_addr))
	{
		if (fault_hidden)
			*fault_hidden = true;
		return false;
	}

	return true;
}

static inline bool compare_image(const std::vector<Interface::RGBA> &reference,
                                 unsigned reference_width, unsigned reference_height,
                                 const std::vector<Interface::RGBA> &gpu, unsigned gpu_width, unsigned gpu_height)
{
	if (reference_width != gpu_width || reference_height != gpu_height)
	{
		LOGE("Reference scanout result resolution does not match GPU. Ref: %u x %u, GPU: %u x %u.\n",
		     reference_width, reference_height,
		     gpu_width, gpu_height);
		return false;
	}

	for (unsigned y = 0; y < reference_height; y++)
	{
		for (unsigned x = 0; x < reference_width; x++)
		{
			auto &a = reference[y * reference_width + x];
			auto &b = gpu[y * reference_width + x];
			if (a.r != b.r || a.g != b.g || a.b != b.b)
			{
				LOGE("Pixel mismatch at %u x %u, [%u, %u, %u] vs [%u, %u, %u]\n",
				     x, y, a.r, a.g, a.b, b.r, b.g, b.b);
				return false;
			}
		}
	}

	return true;
}

static inline void crop_image(std::vector<Interface::RGBA> &reference,
                              int &width, int &height,
                              int left, int right, int top, int bottom)
{
	int new_width = width - left - right;
	int new_height = height - top - bottom;
	assert(new_width > 0 && new_height > 0);
	std::vector<Interface::RGBA> new_reference(new_width * new_height);

	for (int y = 0; y < new_height; y++)
	{
		memcpy(new_reference.data() + y * new_width, reference.data() + (y + top) * width + left,
		       new_width * sizeof(Interface::RGBA));
	}

	reference = std::move(new_reference);
	width = new_width;
	height = new_height;
}

static inline void randomize_rdram(RNG &rng, ReplayerDriver &reference, ReplayerDriver &gpu)
{
	gpu.invalidate_caches();

	auto *rdram_reference = reinterpret_cast<uint32_t *>(reference.get_rdram());
	auto *rdram_gpu = reinterpret_cast<uint32_t *>(gpu.get_rdram());
	size_t size = reference.get_rdram_size() >> 2;

	for (size_t i = 0; i < size; i++)
	{
		auto v = uint32_t(rng.rnd());
		rdram_reference[i] = v;
		rdram_gpu[i] = v;
	}

	rdram_reference = reinterpret_cast<uint32_t *>(reference.get_hidden_rdram());
	rdram_gpu = reinterpret_cast<uint32_t *>(gpu.get_hidden_rdram());
	size = reference.get_hidden_rdram_size() >> 2;

	for (size_t i = 0; i < size; i++)
	{
		auto v = uint32_t(rng.rnd());
		v &= 0x03030303u;
		rdram_reference[i] = v;
		rdram_gpu[i] = v;
	}

	gpu.flush_caches();
}

static inline void clear_rdram(ReplayerDriver &driver)
{
	driver.invalidate_caches();
	memset(driver.get_rdram(), 0, driver.get_rdram_size());
	memset(driver.get_hidden_rdram(), 0, driver.get_hidden_rdram_size());
	driver.flush_caches();
}

static inline bool suite_compare_glob(const std::string &suite, const std::string &cmp)
{
	if (cmp.empty())
		return true;
	return suite.find(cmp) != std::string::npos;
}

static inline bool suite_compare(const std::string &suite, const std::string &cmp)
{
	return suite == cmp;
}

static inline void setup_filesystems()
{
	using namespace Granite;
	using namespace Granite::Path;

#ifdef ANDROID
	filesystem()->register_protocol("rdp", std::make_unique<AssetManagerFilesystem>(""));
	LOGI("Overriding Android RDP filesystem.\n");
#else
	auto exec_path = get_executable_path();
	auto base_dir = basedir(exec_path);
	auto rdp_dir = join(base_dir, "shaders");
	auto builtin_dir = join(base_dir, "builtin");
	auto cache_dir = join(base_dir, "cache");
	bool use_exec_path_cache_dir = false;

	FileStat s = {};
	if (GRANITE_FILESYSTEM()->stat(rdp_dir, s) && s.type == PathType::Directory)
	{
		GRANITE_FILESYSTEM()->register_protocol("rdp", std::make_unique<OSFilesystem>(rdp_dir));
		LOGI("Overriding RDP shader directory to %s.\n", rdp_dir.c_str());
		use_exec_path_cache_dir = true;
	}

	if (GRANITE_FILESYSTEM()->stat(builtin_dir, s) && s.type == PathType::Directory)
	{
		GRANITE_FILESYSTEM()->register_protocol("builtin", std::make_unique<OSFilesystem>(builtin_dir));
		LOGI("Overriding builtin shader directory to %s.\n", builtin_dir.c_str());
		use_exec_path_cache_dir = true;
	}

	if (use_exec_path_cache_dir)
		GRANITE_FILESYSTEM()->register_protocol("cache", std::make_unique<OSFilesystem>(cache_dir));
#endif
}
}
