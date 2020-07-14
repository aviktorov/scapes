#pragma once

#include <unordered_map>
#include <volk.h>

namespace render::backend::vulkan
{
	class Device;
	struct Texture;
	struct SwapChain;

	/*
	 */
	class ImageViewCache
	{
	public:
		ImageViewCache(const Device *device)
			: device(device) { }
		~ImageViewCache();

		VkImageView fetch(const SwapChain *swap_chain, uint32_t base_image);
		VkImageView fetch(const Texture *texture, uint32_t base_mip = 0, uint32_t num_mips = 1, uint32_t base_layer = 0, uint32_t num_layers = 1);
		void clear();

	private:
		uint64_t getHash(
			VkImage image,
			VkFormat format,
			VkImageAspectFlags aspect_flags,
			VkImageViewType view_type,
			uint32_t base_mip = 0,
			uint32_t num_mips = 1,
			uint32_t base_layer = 0,
			uint32_t num_layers = 1
		) const;

	private:
		const Device *device {nullptr};

		std::unordered_map<uint64_t, VkImageView> cache;
	};
}
