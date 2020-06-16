#include "render/backend/vulkan/DescriptorSetCache.h"
#include "render/backend/vulkan/DescriptorSetLayoutCache.h"

#include "render/backend/vulkan/driver.h"
#include "render/backend/vulkan/device.h"

#include <cassert>

namespace render::backend::vulkan
{
	template <class T>
	static void hashCombine(uint64_t &s, const T &v)
	{
		std::hash<T> h;
		s^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
	}

	DescriptorSetCache::~DescriptorSetCache()
	{
		clear();
	}

	VkDescriptorSet DescriptorSetCache::fetch(const BindSet *bind_set)
	{
		uint64_t hash = getHash(bind_set);

		auto it = cache.find(hash);
		if (it != cache.end())
			return it->second;

		VkDescriptorSetLayout layout = layout_cache->fetch(bind_set);

		VkDescriptorSetAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool = device->getDescriptorPool();
		info.descriptorSetCount = 1;
		info.pSetLayouts = &layout;

		VkDescriptorSet result = VK_NULL_HANDLE;
		vkAllocateDescriptorSets(device->getDevice(), &info, &result);
		// TODO: log error

		cache[hash] = result;
		return result;
	}

	void DescriptorSetCache::clear()
	{
		for (auto it = cache.begin(); it != cache.end(); ++it)
			vkFreeDescriptorSets(device->getDevice(), device->getDescriptorPool(), 1, &it->second);

		cache.clear();
	}

	uint64_t DescriptorSetCache::getHash(const BindSet *bind_set) const
	{
		uint64_t hash = 0;
		
		for (uint8_t i = 0; i < BindSet::MAX_BINDINGS; ++i)
		{
			if (!bind_set->binding_used[i])
				continue;

			const VkDescriptorSetLayoutBinding &info = bind_set->bindings[i];
			const vulkan::BindSet::Data &data = bind_set->binding_data[i];

			hashCombine(hash, info.binding);
			hashCombine(hash, info.descriptorType);
			hashCombine(hash, info.stageFlags);
			if (info.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				hashCombine(hash, data.texture.view);
				hashCombine(hash, data.texture.sampler);
			}
			else if (info.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			{
				hashCombine(hash, data.ubo);
			}
		}

		return hash;
	}
}