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

namespace RDP
{
struct SideBySideDriver : ReplayerDriver
{
	SideBySideDriver(ReplayerDriver *first_, ReplayerDriver *second_, ReplayerEventInterface &iface_)
		: first(first_), second(second_), iface(iface_)
	{
	}

	uint8_t *get_rdram() override
	{
		return nullptr;
	}

	size_t get_rdram_size() override
	{
		return 0;
	}

	uint8_t *get_hidden_rdram() override
	{
		return nullptr;
	}

	size_t get_hidden_rdram_size() override
	{
		return 0;
	}

	uint8_t *get_tmem() override
	{
		return nullptr;
	}

	void idle() override
	{
		first->idle();
		second->idle();
	}

	void set_vi_register(VIRegister index, uint32_t value) override;
	void signal_complete() override;
	void command(Op cmd_id, uint32_t num_words, const uint32_t *words) override;
	void end_frame() override;
	void eof() override;
	void update_rdram(const void *data, size_t size, size_t offset) override;
	void update_hidden_rdram(const void *data, size_t size, size_t offset) override;

	ReplayerDriver *first;
	ReplayerDriver *second;
	ReplayerEventInterface &iface;
};

void SideBySideDriver::set_vi_register(VIRegister index, uint32_t value)
{
	iface.set_context_index(0);
	first->set_vi_register(index, value);
	iface.set_context_index(1);
	second->set_vi_register(index, value);
}

void SideBySideDriver::signal_complete()
{
	iface.set_context_index(0);
	first->signal_complete();
	iface.set_context_index(1);
	second->signal_complete();
}

void SideBySideDriver::command(Op cmd_id, uint32_t num_words, const uint32_t *words)
{
	iface.set_context_index(0);
	first->command(cmd_id, num_words, words);
	iface.set_context_index(1);
	second->command(cmd_id, num_words, words);
}

void SideBySideDriver::end_frame()
{
	iface.set_context_index(0);
	first->end_frame();
	iface.set_context_index(1);
	second->end_frame();
}

void SideBySideDriver::eof()
{
	iface.set_context_index(0);
	first->eof();
	iface.set_context_index(1);
	second->eof();
}

void SideBySideDriver::update_rdram(const void *data, size_t size, size_t offset)
{
	iface.set_context_index(0);
	first->update_rdram(data, size, offset);
	iface.set_context_index(1);
	second->update_rdram(data, size, offset);
}

void SideBySideDriver::update_hidden_rdram(const void *data, size_t size, size_t offset)
{
	iface.set_context_index(0);
	first->update_hidden_rdram(data, size, offset);
	iface.set_context_index(1);
	second->update_hidden_rdram(data, size, offset);
}

std::unique_ptr<ReplayerDriver> create_side_by_side_driver(ReplayerDriver *first, ReplayerDriver *second,
                                                           ReplayerEventInterface &iface)
{
	return std::make_unique<SideBySideDriver>(first, second, iface);
}
}