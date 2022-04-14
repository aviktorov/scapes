#include <hardware/vulkan/ImageViewCache.h>

#include <hardware/vulkan/Device.h>
#include <hardware/vulkan/Context.h>
#include <hardware/vulkan/Utils.h>

#include <HashUtils.h>

#include <vector>
#include <cassert>

namespace scapes::visual::hardware::vulkan
{
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

		VkImageView result = Utils::createImageView(context, texture, base_mip, num_mips, base_layer, num_layers);

		cache[hash] = result;
		return result;
	}

	void ImageViewCache::clear()
	{
		for (auto it = cache.begin(); it != cache.end(); ++it)
			vkDestroyImageView(context->getDevice(), it->second, nullptr);

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

		common::HashUtils::combine(hash, texture->image);
		common::HashUtils::combine(hash, texture->format);
		common::HashUtils::combine(hash, aspect_flags);
		common::HashUtils::combine(hash, view_type);
		common::HashUtils::combine(hash, base_mip);
		common::HashUtils::combine(hash, num_mips);
		common::HashUtils::combine(hash, base_layer);
		common::HashUtils::combine(hash, num_layers);

		return hash;
	}
}
