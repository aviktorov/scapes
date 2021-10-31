#pragma once

#include <unordered_map>
#include <volk.h>

namespace scapes::foundation::render::vulkan
{
	struct BindSet;
	class Context;

	/*
	 */
	class DescriptorSetLayoutCache
	{
	public:
		DescriptorSetLayoutCache(const Context *context)
			: context(context) { }
		~DescriptorSetLayoutCache();

		VkDescriptorSetLayout fetch(const BindSet *bind_set);
		void clear();

	private:
		uint64_t getHash(const BindSet *bind_set) const;

	private:
		const Context *context {nullptr};

		std::unordered_map<uint64_t, VkDescriptorSetLayout> cache;
	};
}
