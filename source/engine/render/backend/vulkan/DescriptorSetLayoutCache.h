#pragma once

#include <unordered_map>
#include <volk.h>

namespace render::backend::vulkan
{
	struct BindSet;
	class Device;

	/*
	 */
	class DescriptorSetLayoutCache
	{
	public:
		DescriptorSetLayoutCache(const Device *device)
			: device(device) { }
		~DescriptorSetLayoutCache();

		VkDescriptorSetLayout fetch(const BindSet *bind_set);
		void clear();

	private:
		uint64_t getHash(const BindSet *bind_set) const;

	private:
		const Device *device {nullptr};

		std::unordered_map<uint64_t, VkDescriptorSetLayout> cache;
	};
}
