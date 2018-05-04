/* Copyright (c) 2017-2018 Hans-Kristian Arntzen
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

#include "vulkan.hpp"
#include "intrusive.hpp"
#include <vector>

namespace Vulkan
{
class Device;
class Buffer;

struct BufferBlockAllocation
{
	uint8_t *host;
	VkDeviceSize offset;
};

struct BufferBlock
{
	Util::IntrusivePtr<Buffer> gpu;
	Util::IntrusivePtr<Buffer> cpu;
	VkDeviceSize offset = 0;
	VkDeviceSize alignment = 0;
	VkDeviceSize size = 0;
	uint8_t *mapped = nullptr;

	BufferBlockAllocation allocate(VkDeviceSize allocate_size)
	{
		auto aligned_offset = (offset + alignment - 1) & ~(alignment - 1);
		if (aligned_offset + allocate_size <= size)
		{
			auto *ret = mapped + aligned_offset;
			offset = aligned_offset + allocate_size;
			return { ret, aligned_offset };
		}
		else
			return { nullptr, 0 };
	}
};

class BufferPool
{
public:
	void init(Device *device, VkDeviceSize block_size, VkDeviceSize alignment, VkBufferUsageFlags usage, bool need_device_local);
	void reset();

	VkDeviceSize get_block_size() const
	{
		return block_size;
	}

	BufferBlock request_block(VkDeviceSize minimum_size);
	void recycle_block(BufferBlock &&block);

private:
	Device *device = nullptr;
	VkDeviceSize block_size = 0;
	VkDeviceSize alignment = 0;
	VkBufferUsageFlags usage = 0;
	std::vector<BufferBlock> blocks;
	BufferBlock allocate_block(VkDeviceSize size);
	bool need_device_local = false;
};
}