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

	VkPipelineLayout build();

private:
	const VulkanContext *context {nullptr};

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
};
