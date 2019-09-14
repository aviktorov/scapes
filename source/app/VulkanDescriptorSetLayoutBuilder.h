#pragma once

#include <vector>
#include <volk.h>

#include "VulkanRendererContext.h"

/*
 */
class VulkanDescriptorSetLayoutBuilder
{
public:
	VulkanDescriptorSetLayoutBuilder(const VulkanRendererContext &context)
		: context(context) { }

	inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

	VulkanDescriptorSetLayoutBuilder &addDescriptorBinding(
		VkDescriptorType type,
		VkShaderStageFlags shaderStageFlags,
		int descriptorCount = 1
	);

	void build();

private:
	VulkanRendererContext context;

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	VkDescriptorSetLayout descriptorSetLayout {VK_NULL_HANDLE};
};
