#include "render/backend/vulkan/DescriptorSetLayoutCache.h"

#include "render/backend/vulkan/driver.h"
#include "render/backend/vulkan/device.h"
#include "render/backend/vulkan/VulkanDescriptorSetLayoutBuilder.h"

#include <cassert>

namespace render::backend::vulkan
{
	template <class T>
	static void hashCombine(uint64_t &s, const T &v)
	{
		std::hash<T> h;
		s^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
	}

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

		VulkanDescriptorSetLayoutBuilder builder;

		for (uint8_t i = 0; i < BindSet::MAX_BINDINGS; ++i)
		{
			if (!bind_set->binding_used[i])
				continue;

			const VkDescriptorSetLayoutBinding &info = bind_set->bindings[i];
			builder.addDescriptorBinding(info.descriptorType, info.stageFlags, info.binding);
		}

		VkDescriptorSetLayout result = builder.build(device->getDevice());
		cache[hash] = result;

		return result;
	}

	void DescriptorSetLayoutCache::clear()
	{
		for (auto it = cache.begin(); it != cache.end(); ++it)
			vkDestroyDescriptorSetLayout(device->getDevice(), it->second, nullptr);

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

			hashCombine(hash, info.binding);
			hashCombine(hash, info.descriptorType);
			hashCombine(hash, info.stageFlags);
		}

		return hash;
	}
}
