#pragma once

#include <vector>
#include <volk.h>

class VulkanContext;

/*
 */
class VulkanDescriptorSetLayoutBuilder
{
public:
	VulkanDescriptorSetLayoutBuilder(const VulkanContext *context)
		: context(context) { }

	inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

	VulkanDescriptorSetLayoutBuilder &addDescriptorBinding(
		VkDescriptorType type,
		VkShaderStageFlags shaderStageFlags,
		int descriptorCount = 1
	);

	VkDescriptorSetLayout build();

private:
	const VulkanContext *context {nullptr};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	VkDescriptorSetLayout descriptorSetLayout {VK_NULL_HANDLE};
};
