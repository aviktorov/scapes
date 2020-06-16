#include "render/backend/vulkan/VulkanDescriptorSetLayoutBuilder.h"
#include "render/backend/vulkan/VulkanUtils.h"

VulkanDescriptorSetLayoutBuilder &VulkanDescriptorSetLayoutBuilder::addDescriptorBinding(
	VkDescriptorType type,
	VkShaderStageFlags shaderStageFlags,
	uint32_t binding,
	int descriptorCount
)
{
	VkDescriptorSetLayoutBinding descriptorBinding = {};
	descriptorBinding.binding = binding;
	descriptorBinding.descriptorType = type;
	descriptorBinding.descriptorCount = descriptorCount;
	descriptorBinding.stageFlags = shaderStageFlags;

	bindings.push_back(descriptorBinding);

	return *this;
}

/*
 */
VkDescriptorSetLayout VulkanDescriptorSetLayoutBuilder::build(VkDevice device)
{
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptorSetLayoutInfo.pBindings = bindings.data();

	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Can't create descriptor set layout");

	return descriptorSetLayout;
}
