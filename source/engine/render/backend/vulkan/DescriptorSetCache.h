#pragma once

#include <unordered_map>
#include <volk.h>

namespace render::backend::vulkan
{
	struct BindSet;
	class Device;
	class DescriptorSetLayoutCache;

	/*
	 */
	class DescriptorSetCache
	{
	public:
		DescriptorSetCache(const Device *device, DescriptorSetLayoutCache *layout_cache)
			: device(device), layout_cache(layout_cache) { }
		~DescriptorSetCache();

		VkDescriptorSet fetch(const BindSet *bind_set);
		void clear();

	private:
		uint64_t getHash(const BindSet *bind_set) const;

	private:
		const Device *device {nullptr};
		DescriptorSetLayoutCache *layout_cache {nullptr};

		std::unordered_map<uint64_t, VkDescriptorSet> cache;
	};
}
