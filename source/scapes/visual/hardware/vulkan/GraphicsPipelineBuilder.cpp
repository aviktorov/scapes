#include <hardware/vulkan/GraphicsPipelineBuilder.h>

namespace scapes::visual::hardware::vulkan
{
	/*
	 */
	GraphicsPipelineBuilder &GraphicsPipelineBuilder::addShaderStage(
		VkShaderModule shader,
		VkShaderStageFlagBits stage,
		const char *entry
	)
	{
		VkPipelineShaderStageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.stage = stage;
		info.module = shader;
		info.pName = entry;

		shader_stages.push_back(info);

		return *this;
	}

	/*
	 */
	GraphicsPipelineBuilder &GraphicsPipelineBuilder::addVertexInput(
		const VkVertexInputBindingDescription &binding,
		const std::vector<VkVertexInputAttributeDescription> &attributes
	)
	{
		vertex_input_bindings.push_back(binding);

		for (auto &attribute : attributes)
			vertex_input_attributes.push_back(attribute);

		return *this;
	}

	GraphicsPipelineBuilder &GraphicsPipelineBuilder::setInputAssemblyState(
		VkPrimitiveTopology topology,
		bool primitive_restart
	)
	{
		input_assembly_state = {};
		input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_state.topology = topology;
		input_assembly_state.primitiveRestartEnable = primitive_restart;

		return *this;
	}

	/*
	 */
	GraphicsPipelineBuilder &GraphicsPipelineBuilder::addDynamicState(
		VkDynamicState state
	)
	{
		dynamic_states.push_back(state);

		return *this;
	}

	GraphicsPipelineBuilder &GraphicsPipelineBuilder::addViewport(
		const VkViewport &viewport
	)
	{
		viewports.push_back(viewport);

		return *this;
	}

	GraphicsPipelineBuilder &GraphicsPipelineBuilder::addScissor(
		const VkRect2D &scissor
	)
	{
		scissors.push_back(scissor);

		return *this;
	}

	/*
	 */
	GraphicsPipelineBuilder &GraphicsPipelineBuilder::addBlendColorAttachment(
		bool blend,
		VkBlendFactor src_color_blend_factor,
		VkBlendFactor dst_color_blend_factor,
		VkBlendOp color_blend_op,
		VkBlendFactor src_alpha_blend_factor,
		VkBlendFactor dst_alpha_blend_factor,
		VkBlendOp alpha_blend_op,
		VkColorComponentFlags color_write_mask
	)
	{
		VkPipelineColorBlendAttachmentState blend_attachment = {};
		blend_attachment.blendEnable = blend;
		blend_attachment.srcColorBlendFactor = src_color_blend_factor;
		blend_attachment.dstColorBlendFactor = dst_color_blend_factor;
		blend_attachment.colorBlendOp = color_blend_op;
		blend_attachment.srcAlphaBlendFactor = src_alpha_blend_factor;
		blend_attachment.dstAlphaBlendFactor = dst_alpha_blend_factor;
		blend_attachment.alphaBlendOp = alpha_blend_op;
		blend_attachment.colorWriteMask = color_write_mask;

		color_blend_attachments.push_back(blend_attachment);

		return *this;
	}

	GraphicsPipelineBuilder &GraphicsPipelineBuilder::setRasterizerState(
		bool depth_clamp,
		bool rasterizer_discard,
		VkPolygonMode polygon_mode,
		float line_width,
		VkCullModeFlags cull_mode,
		VkFrontFace front_face,
		bool depth_bias,
		float depth_bias_constant_factor,
		float depth_bias_clamp,
		float depth_bias_slope_factor
	)
	{
		rasterizer_state = {};
		rasterizer_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer_state.depthClampEnable = depth_clamp; // VK_TRUE value require enabling GPU feature
		rasterizer_state.rasterizerDiscardEnable = rasterizer_discard;
		rasterizer_state.polygonMode = polygon_mode; // other modes than VK_POLYGON_MODE_FILL require enabling GPU feature
		rasterizer_state.lineWidth = line_width; // other values require enabling GPU feature
		rasterizer_state.cullMode = cull_mode;
		rasterizer_state.frontFace = front_face;
		rasterizer_state.depthBiasEnable = depth_bias;
		rasterizer_state.depthBiasConstantFactor = depth_bias_constant_factor;
		rasterizer_state.depthBiasClamp = depth_bias_clamp;
		rasterizer_state.depthBiasSlopeFactor = depth_bias_slope_factor;

		return *this;
	}

	GraphicsPipelineBuilder &GraphicsPipelineBuilder::setMultisampleState(
		VkSampleCountFlagBits num_samples,
		bool sample_shading,
		float min_sample_shading
	)
	{
		// Create multisampling state (enabling it requires enabling GPU feature)
		multisampling_state = {};
		multisampling_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling_state.rasterizationSamples = num_samples;
		multisampling_state.sampleShadingEnable = sample_shading;
		multisampling_state.minSampleShading = min_sample_shading;

		multisampling_state.pSampleMask = nullptr; // Optional
		multisampling_state.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling_state.alphaToOneEnable = VK_FALSE; // Optional

		return *this;
	}

	GraphicsPipelineBuilder &GraphicsPipelineBuilder::setDepthStencilState(
		bool depth_test,
		bool depth_write,
		VkCompareOp depth_compare_op
	)
	{
		// Create depth stencil state
		depth_stencil_state = {};
		depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_state.depthTestEnable = depth_test;
		depth_stencil_state.depthWriteEnable = depth_write;
		depth_stencil_state.depthCompareOp = depth_compare_op;

		// TODO: no stencil test at the moment
		depth_stencil_state.stencilTestEnable = VK_FALSE;
		depth_stencil_state.front = {}; // Optional
		depth_stencil_state.back = {}; // Optional

		depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_state.minDepthBounds = 0.0f; // Optional
		depth_stencil_state.maxDepthBounds = 1.0f; // Optional

		return *this;
	}

	/*
	 */
	VkPipeline GraphicsPipelineBuilder::build(VkDevice device)
	{
		VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
		vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_input_bindings.size());
		vertex_input_state.pVertexBindingDescriptions = vertex_input_bindings.data();
		vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size());
		vertex_input_state.pVertexAttributeDescriptions = vertex_input_attributes.data();

		VkPipelineViewportStateCreateInfo viewport_state = {};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = static_cast<uint32_t>(viewports.size());
		viewport_state.pViewports = viewports.data();
		viewport_state.scissorCount = static_cast<uint32_t>(scissors.size());
		viewport_state.pScissors = scissors.data();

		VkPipelineColorBlendStateCreateInfo color_blend_state = {};
		color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_state.attachmentCount = static_cast<uint32_t>(color_blend_attachments.size());
		color_blend_state.pAttachments = color_blend_attachments.data();

		VkPipelineDynamicStateCreateInfo dynamic_state = {};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
		dynamic_state.pDynamicStates = dynamic_states.data();

		// Create graphics pipeline
		VkGraphicsPipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.stageCount = static_cast<uint32_t>(shader_stages.size());
		info.pStages = shader_stages.data();
		info.pVertexInputState = &vertex_input_state;
		info.pInputAssemblyState = &input_assembly_state;
		info.pViewportState = &viewport_state;
		info.pRasterizationState = &rasterizer_state;
		info.pMultisampleState = &multisampling_state;
		info.pDepthStencilState = &depth_stencil_state;
		info.pColorBlendState = &color_blend_state;
		info.pDynamicState = &dynamic_state;
		info.layout = pipeline_layout;
		info.renderPass = render_pass;

		VkPipeline result = VK_NULL_HANDLE;
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &result) != VK_SUCCESS)
		{
			// TODO: log error "Can't create graphics pipeline"
		}

		return result;
	}
}
