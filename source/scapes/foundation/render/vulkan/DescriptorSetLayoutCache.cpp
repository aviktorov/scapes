#include "render/vulkan/DescriptorSetLayoutCache.h"
#include "render/vulkan/DescriptorSetLayoutBuilder.h"

#include "render/vulkan/Device.h"
#include "render/vulkan/Context.h"

#include "HashUtils.h"

#include <cassert>

namespace scapes::foundation::render::vulkan
{
	DescriptorSetLayoutCache::~DescriptorSetLayoutCache()
	{
		clear();
	}

	VkDescriptorSetLayout DescriptorSetLayoutCache::fetch(const BindSet *bind_set)
	{
		uint64_t hash = getHash(bind_set);

		auto it = cache.find(hash);
		if (it != cache.end())
			return it->second;

		DescriptorSetLayoutBuilder builder;

		for (uint8_t i = 0; i < BindSet::MAX_BINDINGS; ++i)
		{
			if (!bind_set->binding_used[i])
				continue;

			const VkDescriptorSetLayoutBinding &info = bind_set->bindings[i];
			builder.addDescriptorBinding(info.descriptorType, info.stageFlags, info.binding);
		}

		VkDescriptorSetLayout result = builder.build(context->getDevice());
		cache[hash] = result;

		return result;
	}

	void DescriptorSetLayoutCache::clear()
	{
		for (auto it = cache.begin(); it != cache.end(); ++it)
			vkDestroyDescriptorSetLayout(context->getDevice(), it->second, nullptr);

		cache.clear();
	}

	uint64_t DescriptorSetLayoutCache::getHash(const BindSet *bind_set) const
	{
		uint64_t hash = 0;
		
		for (uint8_t i = 0; i < BindSet::MAX_BINDINGS; ++i)
		{
			if (!bind_set->binding_used[i])
				continue;

			const VkDescriptorSetLayoutBinding &info = bind_set->bindings[i];

			HashUtils::combine(hash, info.binding);
			HashUtils::combine(hash, info.descriptorType);
			HashUtils::combine(hash, info.stageFlags);
		}

		return hash;
	}
}
