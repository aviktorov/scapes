#pragma once

#include <vector>
#include <volk.h>

/*
 */
class VulkanPipelineLayoutBuilder
{
public:
	VulkanPipelineLayoutBuilder &addDescriptorSetLayout(
		VkDescriptorSetLayout descriptorSetLayout
	);

	VulkanPipelineLayoutBuilder &addPushConstantRange(
		VkShaderStageFlags stageFlags,
		uint32_t offset,
		uint32_t size
	);

	VkPipelineLayout build(VkDevice device);

private:
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	std::vector<VkPushConstantRange> pushConstants;
};
