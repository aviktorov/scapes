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

	VkImageView ImageViewCache::fetch(const SwapChain *swap_chain, uint32_t base_image)
	{
		assert(swap_chain);
		assert(base_image < swap_chain->num_images);

		VkImageAspectFlags aspect_flags = Utils::getImageAspectFlags(swap_chain->surface_format.format);
		VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;

		uint64_t hash = getHash(
			swap_chain->images[base_image],
			swap_chain->surface_format.format,
			aspect_flags,
			view_type
		);

		auto it = cache.find(hash);
		if (it != cache.end())
			return it->second;

		VkImageView result = Utils::createImageView(
			device,
			swap_chain->images[base_image],
			swap_chain->surface_format.format,
			aspect_flags,
			view_type
		);

		cache[hash] = result;
		return result;
	}

	VkImageView ImageViewCache::fetch(const Texture *texture, uint32_t base_mip, uint32_t num_mips, uint32_t base_layer, uint32_t num_layers)
	{
		assert(texture);
		assert(num_layers > 0);
		assert(num_mips > 0);

		VkImageAspectFlags aspect_flags = Utils::getImageAspectFlags(texture->format);
		VkImageViewType view_type = Utils::getImageBaseViewType(texture->type, texture->flags, num_layers);

		uint64_t hash = getHash(
			texture->image,
			texture->format,
			aspect_flags,
			view_type,
			base_mip,
			num_mips,
			base_layer,
			num_layers
		);

		auto it = cache.find(hash);
		if (it != cache.end())
			return it->second;

		VkImageView result = Utils::createImageView(
			device,
			texture->image,
			texture->format,
			aspect_flags,
			view_type,
			base_mip, num_mips,
			base_layer, num_layers
		);

		cache[hash] = result;
		return result;
	}

	void ImageViewCache::clear()
	{
		for (auto it = cache.begin(); it != cache.end(); ++it)
			vkDestroyImageView(device->getDevice(), it->second, nullptr);

		cache.clear();
	}

	uint64_t ImageViewCache::getHash(
			VkImage image,
			VkFormat format,
			VkImageAspectFlags aspect_flags,
			VkImageViewType view_type,
			uint32_t base_mip,
			uint32_t num_mips,
			uint32_t base_layer,
			uint32_t num_layers
	) const
	{
		uint64_t hash = 0;

		hashCombine(hash, image);
		hashCombine(hash, format);
		hashCombine(hash, aspect_flags);
		hashCombine(hash, view_type);
		hashCombine(hash, base_mip);
		hashCombine(hash, num_mips);
		hashCombine(hash, base_layer);
		hashCombine(hash, num_layers);

		return hash;
	}
}
