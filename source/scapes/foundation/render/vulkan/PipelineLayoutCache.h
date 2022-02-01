#pragma once

#include <unordered_map>
#include <volk.h>

namespace scapes::foundation::render::vulkan
{
	class DescriptorSetLayoutCache;
	class Context;
	struct GraphicsPipeline;
	struct RayTracePipeline;

	/*
	 */
	class PipelineLayoutCache
	{
	public:
		PipelineLayoutCache(const Context *context, DescriptorSetLayoutCache *layout_cache)
			: context(context), layout_cache(layout_cache) { }
		~PipelineLayoutCache();

		VkPipelineLayout fetch(const RayTracePipeline *raytrace_pipeline);
		VkPipelineLayout fetch(const GraphicsPipeline *graphics_pipeline);
		void clear();

	private:
		VkPipelineLayout fetch(uint8_t num_layouts, const VkDescriptorSetLayout *layouts, uint8_t push_constants_size);
		uint64_t getHash(uint8_t num_layouts, const VkDescriptorSetLayout *layouts, uint8_t push_constants_size) const;

	private:
		const Context *context {nullptr};
		DescriptorSetLayoutCache *layout_cache {nullptr};

		std::unordered_map<uint64_t, VkPipelineLayout> cache;
	};
}
