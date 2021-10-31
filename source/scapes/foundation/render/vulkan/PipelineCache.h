#pragma once

#include <unordered_map>
#include <volk.h>

namespace scapes::foundation::render::vulkan
{
	class PipelineLayoutCache;
	class Context;
	struct PipelineState;

	/*
	 */
	class PipelineCache
	{
	public:
		PipelineCache(const Context *context, PipelineLayoutCache *layout_cache)
			: context(context), layout_cache(layout_cache) { }
		~PipelineCache();

		VkPipeline fetch(VkPipelineLayout layout, const PipelineState *pipeline_state);
		void clear();

	private:
		uint64_t getHash(VkPipelineLayout layout, const PipelineState *pipeline_state) const;

	private:
		const Context *context {nullptr};
		PipelineLayoutCache *layout_cache {nullptr};

		std::unordered_map<uint64_t, VkPipeline> cache;
	};
}
