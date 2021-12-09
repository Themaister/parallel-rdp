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

extern "C" {
#include "n64video.h"
#include "vdac.h"
#include "msg.h"
void rdp_cmd(uint32_t wid, const uint32_t* args);
// HACK: Poke into hidden RDRAM.
extern uint8_t rdram_hidden[RDRAM_MAX_SIZE / 2];
extern uint8_t *get_tmem(void);
}

#include "replayer_driver.hpp"
#include "logging.hpp"
#include "rdp_common.hpp"
#include <string.h>
#include <stdarg.h>

static n64video_config config = {};

namespace RDP
{

class AngrylionReplayer : public ReplayerDriver
{
public:
	AngrylionReplayer(CommandInterface &player_, ReplayerEventInterface &iface);

	~AngrylionReplayer() override;

	void update_screen(const void *data, unsigned width, unsigned height, unsigned row_length);

	void message(MessageType type, const char *msg);

	uint8_t *get_rdram() override
	{
		return rdram.data();
	}

	size_t get_rdram_size() override
	{
		return rdram.size();
	}

	uint8_t *get_hidden_rdram() override
	{
		return rdram_hidden;
	}

	size_t get_hidden_rdram_size() override
	{
		return sizeof(rdram_hidden);
	}

	uint8_t *get_tmem() override
	{
		return ::get_tmem();
	}

	void idle() override
	{
	}

	void flush_caches() override
	{
	}

	void invalidate_caches() override
	{
	}

private:
	CommandInterface &player;
	ReplayerEventInterface &iface;
	std::vector<uint8_t> rdram;
	uint32_t vi_regs[VI_NUM_REG] = {};
	uint32_t dp_regs[DP_NUM_REG] = {};
	uint32_t irq_reg = 0;

	uint32_t *p_vi_regs[VI_NUM_REG] = {};
	uint32_t *p_dp_regs[DP_NUM_REG] = {};

	void eof() override;
	void signal_complete() override;
	void update_rdram(const void *data, size_t size, size_t offset) override;
	void update_hidden_rdram(const void *data, size_t size, size_t offset) override;
	void command(Op command_id, uint32_t word_count, const uint32_t *words) override;
	void end_frame() override;
	void set_vi_register(VIRegister index, uint32_t value) override;

	void begin_vi_register_per_scanline() override;
	void set_vi_register_for_scanline(unsigned vi_line, uint32_t h_start, uint32_t x_scale) override;
	void end_vi_register_per_scanline() override;
};

static AngrylionReplayer *global_replayer;
}

void vdac_init(struct n64video_config *)
{
}

void vdac_write(struct frame_buffer *fb)
{
	RDP::global_replayer->update_screen(fb->pixels, fb->width, fb->height, fb->pitch);
}

void vdac_sync(bool invalid)
{
	if (invalid)
		RDP::global_replayer->update_screen(nullptr, 0, 0, 0);
}

void vdac_close()
{
}

void msg_error(const char *err, ...)
{
	va_list va;
	va_start(va, err);
	char buffer[16 * 1024];
	vsnprintf(buffer, sizeof(buffer), err, va);
	va_end(va);
	RDP::global_replayer->message(RDP::MessageType::Error, buffer);
}

void msg_warning(const char *err, ...)
{
	va_list va;
	va_start(va, err);
	char buffer[16 * 1024];
	vsnprintf(buffer, sizeof(buffer), err, va);
	va_end(va);
	RDP::global_replayer->message(RDP::MessageType::Warn, buffer);
}

void msg_debug(const char *err, ...)
{
	va_list va;
	va_start(va, err);
	char buffer[16 * 1024];
	vsnprintf(buffer, sizeof(buffer), err, va);
	va_end(va);
	RDP::global_replayer->message(RDP::MessageType::Info, buffer);
}

namespace RDP
{
void AngrylionReplayer::begin_vi_register_per_scanline()
{
}

void AngrylionReplayer::set_vi_register_for_scanline(unsigned, uint32_t, uint32_t)
{
}

void AngrylionReplayer::end_vi_register_per_scanline()
{
}

void AngrylionReplayer::update_screen(const void *data, unsigned width, unsigned height, unsigned row_length)
{
	iface.update_screen(data, width, height, row_length);
}

void AngrylionReplayer::message(MessageType type, const char *msg)
{
	iface.message(type, msg);
}

void AngrylionReplayer::eof()
{
	iface.eof();
}

void AngrylionReplayer::signal_complete()
{
	iface.signal_complete();
}

void AngrylionReplayer::update_rdram(const void *data, size_t size, size_t offset)
{
	memcpy(rdram.data() + offset, data, size);
}

void AngrylionReplayer::update_hidden_rdram(const void *data, size_t size, size_t offset)
{
	memcpy(rdram_hidden + offset, data, size);
}

void AngrylionReplayer::command(Op command_id, uint32_t num_words, const uint32_t *words)
{
	rdp_cmd(0, words);
	iface.notify_command(command_id, num_words, words);
}

void AngrylionReplayer::end_frame()
{
	n64video_update_screen();
}

void AngrylionReplayer::set_vi_register(VIRegister index, uint32_t value)
{
	vi_regs[unsigned(index)] = value;
	//LOGI("Setting VI register %u -> %u.\n", index, value);
}

AngrylionReplayer::AngrylionReplayer(CommandInterface &player_, ReplayerEventInterface &iface_)
	: player(player_), iface(iface_)
{
	rdram.resize(player.get_rdram_size());
	for (unsigned i = 0; i < VI_NUM_REG; i++)
		p_vi_regs[i] = &vi_regs[i];
	for (unsigned i = 0; i < DP_NUM_REG; i++)
		p_dp_regs[i] = &dp_regs[i];

	config.gfx.rdram = rdram.data();
	config.gfx.rdram_size = rdram.size();
	config.gfx.vi_reg = p_vi_regs;
	config.gfx.dp_reg = p_dp_regs;
	config.gfx.mi_intr_reg = &irq_reg;
	config.gfx.mi_intr_cb = []() {};
	config.vi.mode = VI_MODE_NORMAL;
	config.vi.interp = VI_INTERP_LINEAR;
	config.dp.compat = DP_COMPAT_HIGH;
	n64video_init(&config);
}

AngrylionReplayer::~AngrylionReplayer()
{
	n64video_close();
}

std::unique_ptr<ReplayerDriver> create_replayer_driver_angrylion(CommandInterface &player, ReplayerEventInterface &iface)
{
	if (global_replayer)
	{
		LOGE("Angrylion is a singleton renderer.\n");
		return nullptr;
	}

	auto ret = std::make_unique<AngrylionReplayer>(player, iface);
	global_replayer = ret.get();
	return ret;
}
}
