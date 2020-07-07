#pragma once

#include <vector>
#include <volk.h>

namespace render::backend::vulkan
{
	/*
	 */
	class PipelineLayoutBuilder
	{
	public:
		PipelineLayoutBuilder &addDescriptorSetLayout(
			VkDescriptorSetLayout layout
		);

		PipelineLayoutBuilder &addPushConstantRange(
			VkShaderStageFlags shader_stages,
			uint32_t offset,
			uint32_t size
		);

		VkPipelineLayout build(VkDevice device);

	private:
		std::vector<VkDescriptorSetLayout> layouts;
		std::vector<VkPushConstantRange> push_constants;
	};
}
