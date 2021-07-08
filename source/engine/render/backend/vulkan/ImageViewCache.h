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

		VkImageView fetch(const Texture *texture, uint32_t base_mip = 0, uint32_t num_mips = 1, uint32_t base_layer = 0, uint32_t num_layers = 1);
		void clear();

	private:
		uint64_t getHash(const Texture *texture, uint32_t base_mip, uint32_t num_mips,uint32_t base_layer, uint32_t num_layers) const;

	private:
		const Device *device {nullptr};

		std::unordered_map<uint64_t, VkImageView> cache;
	};
}
