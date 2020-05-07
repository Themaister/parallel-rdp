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

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <memory>
#include "rdp_common.hpp"

namespace RDP
{

struct CommandListenerInterface
{
	virtual ~CommandListenerInterface() = default;
	virtual void set_vi_register(VIRegister reg, uint32_t value) = 0;
	virtual void signal_complete() = 0;
	virtual void command(Op cmd_id, uint32_t num_words, const uint32_t *words) = 0;
	virtual void end_frame() = 0;
	virtual void eof() = 0;
	virtual void update_rdram(const void *data, size_t size, size_t offset) = 0;
	virtual void update_hidden_rdram(const void *data, size_t size, size_t offset) = 0;
};

struct CommandInterface
{
	virtual void set_command_interface(CommandListenerInterface *iface) = 0;
	virtual size_t get_rdram_size() const = 0;
	virtual size_t get_hidden_rdram_size() const = 0;
};

class DumpPlayer : public CommandInterface
{
public:
	bool load_dump(const char *path);
	size_t get_rdram_size() const override;
	size_t get_hidden_rdram_size() const override;
	bool iterate();
	bool rewind();
	void set_command_interface(CommandListenerInterface *iface) override;

private:
	CommandListenerInterface *iface = nullptr;

	struct FileDeleter
	{
		void operator()(FILE *f) { fclose(f); }
	};
	std::unique_ptr<FILE, FileDeleter> file;
	std::vector<uint8_t> rdram_cache;
	std::vector<uint8_t> rdram_hidden_cache;
	std::vector<uint32_t> command_buffer;
	bool read_word(uint32_t &value);
};
}