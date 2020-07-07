#pragma once

#include <vector>
#include <volk.h>

namespace render::backend::vulkan
{
	/*
	 */
	class GraphicsPipelineBuilder
	{
	public:
		GraphicsPipelineBuilder(VkPipelineLayout pipeline_layout, VkRenderPass render_pass)
			: render_pass(render_pass), pipeline_layout(pipeline_layout) { }

		GraphicsPipelineBuilder &addShaderStage(
			VkShaderModule shader,
			VkShaderStageFlagBits stage,
			const char *entry = "main"
		);

		GraphicsPipelineBuilder &addVertexInput(
			const VkVertexInputBindingDescription &binding,
			const std::vector<VkVertexInputAttributeDescription> &attributes
		);

		GraphicsPipelineBuilder &setInputAssemblyState(
			VkPrimitiveTopology topology,
			bool primitive_restart = false
		);

		GraphicsPipelineBuilder &addDynamicState(
			VkDynamicState state
		);

		GraphicsPipelineBuilder &addViewport(
			const VkViewport &viewport
		);

		GraphicsPipelineBuilder &addScissor(
			const VkRect2D &scissor
		);

		GraphicsPipelineBuilder &GraphicsPipelineBuilder::addBlendColorAttachment(
			bool blend = false,
			VkBlendFactor src_color_blend_factor = VK_BLEND_FACTOR_ONE,
			VkBlendFactor dst_color_blend_factor = VK_BLEND_FACTOR_ONE,
			VkBlendOp color_blend_op = VK_BLEND_OP_ADD,
			VkBlendFactor src_alpha_blend_factor = VK_BLEND_FACTOR_ONE,
			VkBlendFactor dst_alpha_blend_factor = VK_BLEND_FACTOR_ONE,
			VkBlendOp alpha_blend_op = VK_BLEND_OP_ADD,
			VkColorComponentFlags color_write_mask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		);

		GraphicsPipelineBuilder &setRasterizerState(
			bool depth_clamp,
			bool rasterizer_discard,
			VkPolygonMode polygon_mode,
			float line_width,
			VkCullModeFlags cull_mode,
			VkFrontFace front_face,
			bool depth_bias = false,
			float depth_bias_constant_factor = 0.0f,
			float depth_bias_clamp = 0.0f,
			float depth_bias_slope_factor = 0.0f
		);

		GraphicsPipelineBuilder &setMultisampleState(
			VkSampleCountFlagBits num_samples,
			bool sample_shading = false,
			float min_sample_shading = 1.0f
		);

		GraphicsPipelineBuilder &setDepthStencilState(
			bool depth_test,
			bool depth_write,
			VkCompareOp depth_compare_op
		);

		VkPipeline build(VkDevice device);

	private:
		VkRenderPass render_pass {VK_NULL_HANDLE};
		VkPipelineLayout pipeline_layout {VK_NULL_HANDLE};

		std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
		std::vector<VkVertexInputBindingDescription> vertex_input_bindings;
		std::vector<VkVertexInputAttributeDescription> vertex_input_attributes;
		std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;

		std::vector<VkDynamicState> dynamic_states;
		std::vector<VkViewport> viewports;
		std::vector<VkRect2D> scissors;

		VkPipelineInputAssemblyStateCreateInfo input_assembly_state {};
		VkPipelineRasterizationStateCreateInfo rasterizer_state {};
		VkPipelineMultisampleStateCreateInfo multisampling_state {};
		VkPipelineDepthStencilStateCreateInfo depth_stencil_state {};
	};
}
