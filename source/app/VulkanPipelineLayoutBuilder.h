#pragma once

#include <vector>
#include <volk.h>

#include "VulkanRendererContext.h"

/*
 */
class VulkanPipelineLayoutBuilder
{
public:
	VulkanPipelineLayoutBuilder(const VulkanRendererContext &context)
		: context(context) { }

	inline VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

	VulkanPipelineLayoutBuilder &addDescriptorSetLayout(
		VkDescriptorSetLayout descriptorSetLayout
	);

	VkPipelineLayout build();

private:
	VulkanRendererContext context;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
};
