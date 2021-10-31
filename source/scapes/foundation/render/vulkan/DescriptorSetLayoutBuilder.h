#pragma once

#include <vector>
#include <volk.h>

namespace scapes::foundation::render::vulkan
{
	/*
	 */
	class DescriptorSetLayoutBuilder
	{
	public:
		DescriptorSetLayoutBuilder &addDescriptorBinding(
			VkDescriptorType type,
			VkShaderStageFlags shader_stages,
			uint32_t binding,
			int descriptor_count = 1
		);

		VkDescriptorSetLayout build(VkDevice device);

	private:
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};
}
