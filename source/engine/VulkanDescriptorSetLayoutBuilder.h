#pragma once

#include <vector>
#include <volk.h>

/*
 */
class VulkanDescriptorSetLayoutBuilder
{
public:
	VulkanDescriptorSetLayoutBuilder &addDescriptorBinding(
		VkDescriptorType type,
		VkShaderStageFlags shaderStageFlags,
		int descriptorCount = 1
	);

	VkDescriptorSetLayout build(VkDevice device);

private:
	std::vector<VkDescriptorSetLayoutBinding> bindings;
};
