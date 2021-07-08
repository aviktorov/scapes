#include "render/backend/vulkan/ImageViewCache.h"

#include "render/backend/vulkan/Driver.h"
#include "render/backend/vulkan/Device.h"
#include "render/backend/vulkan/Utils.h"

#include <vector>
#include <cassert>

namespace render::backend::vulkan
{
	template <class T>
	static void hashCombine(uint64_t &s, const T &v)
	{
		std::hash<T> h;
		s^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
	}

	ImageViewCache::~ImageViewCache()
	{
		clear();
	}

	VkImageView ImageViewCache::fetch(const Texture *texture, uint32_t base_mip, uint32_t num_mips, uint32_t base_layer, uint32_t num_layers)
	{
		assert(texture);
		assert(num_layers > 0);
		assert(num_mips > 0);
		assert(base_mip + num_mips <= texture->num_mipmaps);
		assert(base_layer + num_layers <= texture->num_layers);

		uint64_t hash = getHash(texture, base_mip, num_mips, base_layer, num_layers);

		auto it = cache.find(hash);
		if (it != cache.end())
			return it->second;

		VkImageView result = Utils::createImageView(device, texture, base_mip, num_mips, base_layer, num_layers);

		cache[hash] = result;
		return result;
	}

	void ImageViewCache::clear()
	{
		for (auto it = cache.begin(); it != cache.end(); ++it)
			vkDestroyImageView(device->getDevice(), it->second, nullptr);

		cache.clear();
	}

	uint64_t ImageViewCache::getHash(const Texture *texture, uint32_t base_mip, uint32_t num_mips, uint32_t base_layer, uint32_t num_layers) const
	{
		assert(texture);
		assert(num_layers > 0);
		assert(num_mips > 0);
		assert(base_mip + num_mips <= texture->num_mipmaps);
		assert(base_layer + num_layers <= texture->num_layers);

		VkImageAspectFlags aspect_flags = Utils::getImageAspectFlags(texture->format);
		VkImageViewType view_type = Utils::getImageBaseViewType(texture->type, texture->flags, num_layers);

		uint64_t hash = 0;

		hashCombine(hash, texture->image);
		hashCombine(hash, texture->format);
		hashCombine(hash, aspect_flags);
		hashCombine(hash, view_type);
		hashCombine(hash, base_mip);
		hashCombine(hash, num_mips);
		hashCombine(hash, base_layer);
		hashCombine(hash, num_layers);

		return hash;
	}
}
