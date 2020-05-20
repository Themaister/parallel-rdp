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

#include <memory>
#include "rdp_dump.hpp"

namespace Vulkan
{
class Device;
}

namespace RDP
{
enum class MessageType
{
	Info,
	Warn,
	Error
};

static inline const char *command_name(Op cmd_id)
{
	auto index = unsigned(cmd_id);

	static const char *names[64] = {
		/* 0x00 */ "NOP", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		/* 0x08 */ "TRI", "ZBUF_TRI", "TEX_TRI", "TEX_Z_TRI", "SHADE_TRI", "SHADE_Z_TRI", "SHADE_TEX_TRI", "SHADE_TEX_Z_TRI",
		/* 0x10 */ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		/* 0x18 */ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		/* 0x20 */ nullptr, nullptr, nullptr, nullptr, "TEX_RECT", "TEX_RECT_FLIP", "SYNC_LOAD", "SYNC_PIPE",
		/* 0x28 */ "SYNC_TILE", "SYNC_FULL", "SET_KEY_GB", "SET_KEY_R", "SET_CONVERT", "SET_SCISSOR", "SET_PRIM_DEPTH", "SET_OTHER",
		/* 0x30 */ "LOAD_TLUT", nullptr, "SET_TILE_SIZE", "LOAD_BLOCK", "LOAD_TILE", "SET_TILE", "FILL_RECT", "SET_FILL_COLOR",
		/* 0x38 */ "SET_FOG_COLOR", "SET_BLEND_COLOR", "SET_PRIM_COLOR", "SET_ENV_COLOR", "SET_COMBINE", "SET_TEX_IMAGE", "SET_MASK_IMAGE", "SET_COLOR_IMAGE",
	};

	if (index < 64 && names[index])
		return names[index];
	else
		return "???";
}

class ReplayerDriver : public CommandListenerInterface
{
public:
	virtual ~ReplayerDriver() = default;
	virtual uint8_t *get_rdram() = 0;
	virtual size_t get_rdram_size() = 0;
	virtual uint8_t *get_hidden_rdram() = 0;
	virtual size_t get_hidden_rdram_size() = 0;
	virtual uint8_t *get_tmem() = 0;
	virtual void idle() = 0;

	virtual void flush_caches() = 0;
	virtual void invalidate_caches() = 0;
};

static inline bool command_is_draw_call(Op cmd_id)
{
	switch (cmd_id)
	{
	case Op::FillTriangle:
	case Op::TextureZBufferTriangle:
	case Op::TextureTriangle:
	case Op::FillZBufferTriangle:
	case Op::ShadeTriangle:
	case Op::ShadeZBufferTriangle:
	case Op::ShadeTextureTriangle:
	case Op::ShadeTextureZBufferTriangle:
	case Op::TextureRectangle:
	case Op::TextureRectangleFlip:
	case Op::FillRectangle:
		return true;

	default:
		return false;
	}
}

struct ReplayerEventInterface
{
	virtual void update_screen(const void *data, unsigned width, unsigned height, unsigned row_length) = 0;
	virtual void notify_command(Op cmd_id, uint32_t num_words, const uint32_t *words) = 0;
	virtual void message(MessageType type, const char *msg) = 0;
	virtual void eof() = 0;
	virtual void set_context_index(unsigned index) = 0;
	virtual void signal_complete() = 0;
};

std::unique_ptr<ReplayerDriver> create_replayer_driver_angrylion(CommandInterface &player, ReplayerEventInterface &iface);
std::unique_ptr<ReplayerDriver> create_replayer_driver_parallel(Vulkan::Device &device, CommandInterface &player, ReplayerEventInterface &iface);
std::unique_ptr<ReplayerDriver> create_side_by_side_driver(ReplayerDriver *first, ReplayerDriver *second, ReplayerEventInterface &iface);
}