#pragma once

#include <unordered_map>
#include <volk.h>

namespace render::backend::vulkan
{
	class DescriptorSetLayoutCache;
	class Device;
	struct PipelineState;

	/*
	 */
	class PipelineLayoutCache
	{
	public:
		PipelineLayoutCache(const Device *device, DescriptorSetLayoutCache *layout_cache)
			: device(device), layout_cache(layout_cache) { }
		~PipelineLayoutCache();

		VkPipelineLayout fetch(const PipelineState *pipeline_state);
		void clear();

	private:
		uint64_t getHash(uint8_t num_layouts, const VkDescriptorSetLayout *layouts, uint8_t push_constants_size) const;

	private:
		const Device *device {nullptr};
		DescriptorSetLayoutCache *layout_cache {nullptr};

		std::unordered_map<uint64_t, VkPipelineLayout> cache;
	};
}
