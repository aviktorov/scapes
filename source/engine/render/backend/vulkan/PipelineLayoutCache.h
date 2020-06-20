#pragma once

#include <unordered_map>
#include <volk.h>

namespace render::backend::vulkan
{
	class DescriptorSetLayoutCache;
	class Device;
	class Context;

	/*
	 */
	class PipelineLayoutCache
	{
	public:
		PipelineLayoutCache(const Device *device, DescriptorSetLayoutCache *layout_cache)
			: device(device), layout_cache(layout_cache) { }
		~PipelineLayoutCache();

		VkPipelineLayout fetch(const Context *context);
		void clear();

	private:
		uint64_t getHash(const uint8_t num_layouts, const VkDescriptorSetLayout *layouts) const;

	private:
		const Device *device {nullptr};
		DescriptorSetLayoutCache *layout_cache {nullptr};

		std::unordered_map<uint64_t, VkPipelineLayout> cache;
	};
}
