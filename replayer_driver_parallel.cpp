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
#include "device.hpp"
#include "rdp_device.hpp"
#include "aligned_alloc.hpp"

namespace RDP
{
class ParallelReplayer : public ReplayerDriver
{
public:
	ParallelReplayer(Vulkan::Device &device, CommandInterface &player_,
	                 ReplayerEventInterface &iface_, bool benchmarking)
		: player(player_)
		, iface(iface_)
		, host_memory(Util::memalign_calloc(64 * 1024, player.get_rdram_size()))
		, gpu(device, host_memory.get(), 0, player.get_rdram_size(), player.get_hidden_rdram_size(),
			  benchmarking ? 0 : (COMMAND_PROCESSOR_FLAG_HOST_VISIBLE_HIDDEN_RDRAM_BIT | COMMAND_PROCESSOR_FLAG_HOST_VISIBLE_TMEM_BIT))
	{
		if (!gpu.device_is_supported())
			throw std::runtime_error("GPU is not supported.");
	}

private:
	CommandInterface &player;
	ReplayerEventInterface &iface;
	struct AlignedDeleter
	{
		void operator()(void *ptr)
		{
			Util::memalign_free(ptr);
		}
	};
	std::unique_ptr<void, AlignedDeleter> host_memory;
	CommandProcessor gpu;

	void eof() override;
	void signal_complete() override;
	void update_rdram(const void *data, size_t size, size_t offset) override;
	void update_hidden_rdram(const void *data, size_t size, size_t offset) override;
	void command(Op command_id, uint32_t word_count, const uint32_t *words) override;
	void end_frame() override;
	void set_vi_register(VIRegister index, uint32_t value) override;
	uint8_t *get_rdram() override;
	size_t get_rdram_size() override;
	uint8_t *get_hidden_rdram() override;
	size_t get_hidden_rdram_size() override;
	uint8_t *get_tmem() override;
	void idle() override;

	void invalidate_caches() override;
	void flush_caches() override;

	void begin_vi_register_per_scanline() override;
	void set_vi_register_for_scanline(unsigned vi_line, uint32_t h_start, uint32_t x_scale) override;
	void end_vi_register_per_scanline() override;
};

void ParallelReplayer::begin_vi_register_per_scanline()
{
	gpu.begin_vi_register_per_scanline(VideoInterface::PER_SCANLINE_HSTART_BIT |
	                                   VideoInterface::PER_SCANLINE_XSCALE_BIT);
}

void ParallelReplayer::set_vi_register_for_scanline(unsigned vi_line, uint32_t h_start, uint32_t x_scale)
{
	gpu.set_vi_register_for_scanline(VideoInterface::PER_SCANLINE_HSTART_BIT, h_start);
	gpu.set_vi_register_for_scanline(VideoInterface::PER_SCANLINE_XSCALE_BIT, x_scale);
	gpu.latch_vi_register_for_scanline(vi_line);
}

void ParallelReplayer::end_vi_register_per_scanline()
{
	gpu.end_vi_register_per_scanline();
}

void ParallelReplayer::eof()
{
	iface.eof();
}

void ParallelReplayer::signal_complete()
{
	gpu.flush();
	iface.signal_complete();
}

void ParallelReplayer::update_rdram(const void *data, size_t size, size_t offset)
{
	gpu.idle();
	memcpy(static_cast<uint8_t *>(host_memory.get()) + offset, data, size);
	gpu.end_write_rdram();
}

void ParallelReplayer::flush_caches()
{
	gpu.end_write_rdram();
	gpu.end_write_hidden_rdram();
}

void ParallelReplayer::invalidate_caches()
{
	gpu.begin_read_rdram();
	gpu.begin_read_hidden_rdram();
}

void ParallelReplayer::update_hidden_rdram(const void *data, size_t size, size_t offset)
{
	gpu.idle();
	memcpy(static_cast<uint8_t *>(gpu.begin_read_hidden_rdram()) + offset, data, size);
	gpu.end_write_hidden_rdram();
}

void ParallelReplayer::command(Op command_id, uint32_t num_words, const uint32_t *words)
{
	gpu.enqueue_command(num_words, words);
	iface.notify_command(command_id, num_words, words);
}

uint8_t *ParallelReplayer::get_rdram()
{
	gpu.idle();
	return static_cast<uint8_t *>(host_memory.get());
}

size_t ParallelReplayer::get_rdram_size()
{
	return gpu.get_rdram_size();
}

uint8_t *ParallelReplayer::get_hidden_rdram()
{
	gpu.idle();
	return static_cast<uint8_t *>(gpu.begin_read_hidden_rdram());
}

size_t ParallelReplayer::get_hidden_rdram_size()
{
	return gpu.get_hidden_rdram_size();
}

uint8_t *ParallelReplayer::get_tmem()
{
	gpu.idle();
	return static_cast<uint8_t *>(gpu.get_tmem());
}

void ParallelReplayer::idle()
{
	gpu.idle();
}

void ParallelReplayer::end_frame()
{
	std::vector<RGBA> colors;
	unsigned width, height;
	gpu.scanout_sync(colors, width, height);
	iface.update_screen(colors.data(), width, height, width);
}

void ParallelReplayer::set_vi_register(VIRegister index, uint32_t value)
{
	gpu.set_vi_register(index, value);
}

std::unique_ptr<ReplayerDriver> create_replayer_driver_parallel(Vulkan::Device &device, CommandInterface &player, ReplayerEventInterface &iface,
                                                                bool benchmarking)
{
	return std::make_unique<ParallelReplayer>(device, player, iface, benchmarking);
}
}
