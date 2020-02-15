#include "VulkanPipelineLayoutBuilder.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"

VulkanPipelineLayoutBuilder &VulkanPipelineLayoutBuilder::addDescriptorSetLayout(
	VkDescriptorSetLayout descriptorSetLayout
)
{
	descriptorSetLayouts.push_back(descriptorSetLayout);
	return *this;
}

VulkanPipelineLayoutBuilder &VulkanPipelineLayoutBuilder::addPushConstantRange(
	VkShaderStageFlags stageFlags,
	uint32_t offset,
	uint32_t size
)
{
	VkPushConstantRange range = {};
	range.stageFlags = stageFlags;
	range.offset = offset;
	range.size = size;

	pushConstants.push_back(range);
	return *this;
}

/*
 */
VkPipelineLayout VulkanPipelineLayoutBuilder::build()
{
	// Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
	pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

	if (vkCreatePipelineLayout(context->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Can't create pipeline layout");

	return pipelineLayout;
}
