#include <hardware/vulkan/PipelineLayoutBuilder.h>

namespace scapes::visual::hardware::vulkan
{
	/*
	 */
	PipelineLayoutBuilder &PipelineLayoutBuilder::addDescriptorSetLayout(
		VkDescriptorSetLayout layout
	)
	{
		layouts.push_back(layout);
		return *this;
	}

	PipelineLayoutBuilder &PipelineLayoutBuilder::addPushConstantRange(
		VkShaderStageFlags shader_stages,
		uint32_t offset,
		uint32_t size
	)
	{
		VkPushConstantRange range = {};
		range.stageFlags = shader_stages;
		range.offset = offset;
		range.size = size;

		push_constants.push_back(range);
		return *this;
	}

	/*
	 */
	VkPipelineLayout PipelineLayoutBuilder::build(VkDevice device)
	{
		// Create pipeline layout
		VkPipelineLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount = static_cast<uint32_t>(layouts.size());
		info.pSetLayouts = layouts.data();
		info.pushConstantRangeCount = static_cast<uint32_t>(push_constants.size());
		info.pPushConstantRanges = push_constants.data();

		VkPipelineLayout result = VK_NULL_HANDLE;
		if (vkCreatePipelineLayout(device, &info, nullptr, &result) != VK_SUCCESS)
		{
			// TODO: log error "Can't create pipeline layout"
		}

		return result;
	}
}
