#pragma once

#include <unordered_map>
#include <volk.h>

namespace scapes::foundation::render::vulkan
{
	class PipelineLayoutCache;
	class Context;
	struct GraphicsPipeline;
	struct RayTracePipeline;

	/*
	 */
	class PipelineCache
	{
	public:
		PipelineCache(const Context *context, PipelineLayoutCache *layout_cache)
			: context(context), layout_cache(layout_cache) { }
		~PipelineCache();

		VkPipeline fetch(VkPipelineLayout layout, const GraphicsPipeline *graphics_pipeline);
		VkPipeline fetch(VkPipelineLayout layout, const RayTracePipeline *raytrace_pipeline);
		void clear();

	private:
		uint64_t getHash(VkPipelineLayout layout, const RayTracePipeline *raytrace_pipeline) const;
		uint64_t getHash(VkPipelineLayout layout, const GraphicsPipeline *graphics_pipeline) const;

	private:
		const Context *context {nullptr};
		PipelineLayoutCache *layout_cache {nullptr};

		std::unordered_map<uint64_t, VkPipeline> graphics_pipeline_cache;
		std::unordered_map<uint64_t, VkPipeline> raytrace_pipeline_cache;
	};
}
