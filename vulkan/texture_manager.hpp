#pragma once

#include "volatile_source.hpp"
#include "image.hpp"

namespace Vulkan
{
class Texture : public Util::VolatileSource<Texture>
{
public:
	Texture(Device *device, const std::string &path, VkFormat format = VK_FORMAT_UNDEFINED);

	ImageHandle get_image()
	{
		VK_ASSERT(handle);
		return handle;
	}

	void load();
	void unload();

	void update(const void *data, size_t size);

private:
	Device *device;
	ImageHandle handle;
	VkFormat format;
	void update_png(const void *data, size_t size);
	void update_gli(const void *data, size_t size);
};

class TextureManager
{
public:
	TextureManager(Device *device);
	Texture *request_texture(const std::string &path);

private:
	Device *device;
	std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
};
}