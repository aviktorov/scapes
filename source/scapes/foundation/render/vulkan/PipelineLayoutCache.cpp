#include "render/vulkan/PipelineLayoutCache.h"
#include "render/vulkan/PipelineLayoutBuilder.h"

#include "render/vulkan/Device.h"
#include "render/vulkan/Context.h"

#include "HashUtils.h"

#include <array>
#include <vector>
#include <cassert>

namespace scapes::foundation::render::vulkan
{
	PipelineLayoutCache::~PipelineLayoutCache()
	{
		clear();
	}

	VkPipelineLayout PipelineLayoutCache::fetch(const GraphicsPipeline *graphics_pipeline)
	{
		uint8_t push_constants_size = graphics_pipeline->push_constants_size;
		uint8_t num_bind_sets = graphics_pipeline->num_bind_sets;

		VkDescriptorSetLayout layouts[GraphicsPipeline::MAX_BIND_SETS];
		for (uint8_t i = 0; i < num_bind_sets; ++i)
		{
			BindSet *bind_set = graphics_pipeline->bind_sets[i];
			assert(bind_set);

			layouts[i] = bind_set->set_layout;
		}

		uint64_t hash = getHash(num_bind_sets, layouts, push_constants_size);

		auto it = cache.find(hash);
		if (it != cache.end())
			return it->second;

		PipelineLayoutBuilder builder;

		for (uint8_t i = 0; i < num_bind_sets; ++i)
			builder.addDescriptorSetLayout(layouts[i]);

		if (push_constants_size > 0)
			builder.addPushConstantRange(VK_SHADER_STAGE_ALL, 0, push_constants_size);

		VkPipelineLayout result = builder.build(context->getDevice());

		cache[hash] = result;
		return result;
	}

	void PipelineLayoutCache::clear()
	{
		for (auto it = cache.begin(); it != cache.end(); ++it)
			vkDestroyPipelineLayout(context->getDevice(), it->second, nullptr);

		cache.clear();
	}

	uint64_t PipelineLayoutCache::getHash(uint8_t num_layouts, const VkDescriptorSetLayout *layouts, uint8_t push_constants_size) const
	{
		assert(num_layouts == 0 || layouts != nullptr);

		uint64_t hash = 0;
		common::HashUtils::combine(hash, push_constants_size);
		common::HashUtils::combine(hash, num_layouts);
		
		for (uint8_t i = 0; i < num_layouts; ++i)
			common::HashUtils::combine(hash, layouts[i]);

		return hash;
	}
}
