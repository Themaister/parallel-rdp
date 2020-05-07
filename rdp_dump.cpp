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

#include "rdp_dump.hpp"
#include <string.h>
#include <algorithm>

namespace RDP
{
enum class Command : uint32_t
{
	Invalid = 0,
	UpdateDram = 1,
	RDPCommand = 2,
	SetVIRegister = 3,
	EndFrame = 4,
	SignalComplete = 5,
	EndOfFile = 6,
	UpdateDramFlush = 7,
	UpdateHiddenDram = 8,
	UpdateHiddenDramFlush = 9
};

bool DumpPlayer::load_dump(const char *path)
{
	file.reset(fopen(path, "rb"));
	if (!file)
		return false;

	char header[8];
	if (fread(header, 1, 8, file.get()) != 8)
		return false;

	if (memcmp(header, "RDPDUMP2", 8) != 0)
		return false;

	uint32_t rdram_size, hidden_dram_size;
	if (!read_word(rdram_size) || !read_word(hidden_dram_size))
		return false;

	if (rdram_size != 4 * 1024 * 1024 && rdram_size != 8 * 1024 * 1024)
		return false;
	if (hidden_dram_size != 4 * 1024 * 1024)
		return false;

	rdram_cache.resize(rdram_size);
	rdram_hidden_cache.resize(hidden_dram_size);
	return true;
}

bool DumpPlayer::rewind()
{
	if (fseek(file.get(), 16, SEEK_SET) == 0)
	{
		std::fill(rdram_cache.begin(), rdram_cache.end(), 0);
		std::fill(rdram_hidden_cache.begin(), rdram_hidden_cache.end(), 0);
		return true;
	}
	else
		return false;
}

void DumpPlayer::set_command_interface(CommandListenerInterface *iface_)
{
	iface = iface_;
}

bool DumpPlayer::iterate()
{
	uint32_t command_u32;
	if (!read_word(command_u32))
		return false;
	auto command = static_cast<Command>(command_u32);

	switch (command)
	{
	case Command::EndOfFile:
		iface->eof();
		return false;

	case Command::SetVIRegister:
	{
		uint32_t index, value;
		if (!read_word(index) || !read_word(value))
			return false;

		iface->set_vi_register(VIRegister(index), value);
		break;
	}

	case Command::RDPCommand:
	{
		uint32_t cmd_id;
		if (!read_word(cmd_id))
			return false;
		uint32_t word_count;
		if (!read_word(word_count))
			return false;

		command_buffer.resize(word_count);
		if (word_count)
		{
			if (fread(command_buffer.data(), sizeof(uint32_t), word_count, file.get()) != word_count)
				return false;
		}

		iface->command(static_cast<Op>(cmd_id), word_count, command_buffer.data());
		break;
	}

	case Command::EndFrame:
		iface->end_frame();
		break;

	case Command::SignalComplete:
		iface->signal_complete();
		break;

	case Command::UpdateDram:
	{
		uint32_t offset, size;
		if (!read_word(offset) || !read_word(size))
			return false;

		if (offset + size > rdram_cache.size())
			return false;

		if (fread(rdram_cache.data() + offset, 1, size, file.get()) != size)
			return false;

		break;
	}

	case Command::UpdateHiddenDram:
	{
		uint32_t offset, size;
		if (!read_word(offset) || !read_word(size))
			return false;

		if (offset + size > rdram_hidden_cache.size())
			return false;

		if (fread(rdram_hidden_cache.data() + offset, 1, size, file.get()) != size)
			return false;

		break;
	}

	case Command::UpdateDramFlush:
	{
		iface->update_rdram(rdram_cache.data(), rdram_cache.size(), 0);
		break;
	}

	case Command::UpdateHiddenDramFlush:
	{
		iface->update_hidden_rdram(rdram_hidden_cache.data(), rdram_hidden_cache.size(), 0);
		break;
	}

	default:
		return false;
	}

	return true;
}

bool DumpPlayer::read_word(uint32_t &value)
{
	return fread(&value, sizeof(value), 1, file.get()) == 1;
}

size_t DumpPlayer::get_rdram_size() const
{
	return rdram_cache.size();
}

size_t DumpPlayer::get_hidden_rdram_size() const
{
	return rdram_hidden_cache.size();
}
}
