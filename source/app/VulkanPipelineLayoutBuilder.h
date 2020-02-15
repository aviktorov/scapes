#pragma once

#include <vector>
#include <volk.h>

class VulkanContext;

/*
 */
class VulkanPipelineLayoutBuilder
{
public:
	VulkanPipelineLayoutBuilder(const VulkanContext *context)
		: context(context) { }

	inline VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

	VulkanPipelineLayoutBuilder &addDescriptorSetLayout(
		VkDescriptorSetLayout descriptorSetLayout
	);

	VulkanPipelineLayoutBuilder &addPushConstantRange(
		VkShaderStageFlags stageFlags,
		uint32_t offset,
		uint32_t size
	);

	VkPipelineLayout build();

private:
	const VulkanContext *context {nullptr};

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	std::vector<VkPushConstantRange> pushConstants;

	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
};
