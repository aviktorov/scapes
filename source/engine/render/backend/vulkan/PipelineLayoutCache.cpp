#include "render/backend/vulkan/PipelineLayoutCache.h"

#include "render/backend/vulkan/driver.h"
#include "render/backend/vulkan/device.h"
#include "render/backend/vulkan/context.h"
#include "render/backend/vulkan/DescriptorSetLayoutCache.h"
#include "render/backend/vulkan/VulkanPipelineLayoutBuilder.h"

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

	PipelineLayoutCache::~PipelineLayoutCache()
	{
		clear();
	}

	VkPipelineLayout PipelineLayoutCache::fetch(const Context *context)
	{
		std::vector<VkDescriptorSetLayout> layouts(context->getNumBindSets());
		for (uint8_t i = 0; i < context->getNumBindSets(); ++i)
			layouts[i] = layout_cache->fetch(context->getBindSet(i));

		uint64_t hash = getHash(static_cast<uint8_t>(layouts.size()), layouts.data());

		auto it = cache.find(hash);
		if (it != cache.end())
			return it->second;

		VulkanPipelineLayoutBuilder builder;

		for (size_t i = 0; i < layouts.size(); ++i)
			builder.addDescriptorSetLayout(layouts[i]);

		// TODO: push contants

		VkPipelineLayout result = builder.build(device->getDevice());

		cache[hash] = result;
		return result;
	}

	void PipelineLayoutCache::clear()
	{
		for (auto it = cache.begin(); it != cache.end(); ++it)
			vkDestroyPipelineLayout(device->getDevice(), it->second, nullptr);

		cache.clear();
	}

	uint64_t PipelineLayoutCache::getHash(const uint8_t num_layouts, const VkDescriptorSetLayout *layouts) const
	{
		assert(num_layouts == 0 || layouts != nullptr);

		uint64_t hash = 0;
		hashCombine(hash, num_layouts);
		
		for (uint8_t i = 0; i < num_layouts; ++i)
			hashCombine(hash, layouts[i]);

		return hash;
	}
}
