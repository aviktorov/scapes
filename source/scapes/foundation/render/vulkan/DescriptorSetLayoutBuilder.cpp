#include "render/vulkan/DescriptorSetLayoutBuilder.h"

namespace scapes::foundation::render::vulkan
{
	/*
	 */
	DescriptorSetLayoutBuilder &DescriptorSetLayoutBuilder::addDescriptorBinding(
		VkDescriptorType type,
		VkShaderStageFlags shader_stages,
		uint32_t binding,
		int descriptor_count
	)
	{
		VkDescriptorSetLayoutBinding desciptor_binding = {};
		desciptor_binding.binding = binding;
		desciptor_binding.descriptorType = type;
		desciptor_binding.descriptorCount = descriptor_count;
		desciptor_binding.stageFlags = shader_stages;

		bindings.push_back(desciptor_binding);

		return *this;
	}

	/*
	 */
	VkDescriptorSetLayout DescriptorSetLayoutBuilder::build(VkDevice device)
	{
		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount = static_cast<uint32_t>(bindings.size());
		info.pBindings = bindings.data();

		VkDescriptorSetLayout result = VK_NULL_HANDLE;
		if (vkCreateDescriptorSetLayout(device, &info, nullptr, &result) != VK_SUCCESS)
		{
			// TODO: log error "Can't create descriptor set layout"
		}

		return result;
	}
}
