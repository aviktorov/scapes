#include "VulkanGraphicsPipelineBuilder.h"
#include "VulkanUtils.h"

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::addShaderStage(
	VkShaderModule shader,
	VkShaderStageFlagBits stage,
	const char *entry
)
{
	VkPipelineShaderStageCreateInfo shaderStageInfo = {};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.stage = stage;
	shaderStageInfo.module = shader;
	shaderStageInfo.pName = entry;

	shaderStages.push_back(shaderStageInfo);

	return *this;
}

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::addVertexInput(
	const VkVertexInputBindingDescription &binding,
	const std::vector<VkVertexInputAttributeDescription> &attributes
)
{
	vertexInputBindings.push_back(binding);

	for (auto &attribute : attributes)
		vertexInputAttributes.push_back(attribute);

	return *this;
}

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::addViewport(
	const VkViewport &viewport
)
{
	viewports.push_back(viewport);

	return *this;
}

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::addScissor(
	const VkRect2D &scissor
)
{
	scissors.push_back(scissor);

	return *this;
}

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::addBlendColorAttachment(
	bool blend,
	VkBlendFactor srcColorBlendFactor,
	VkBlendFactor dstColorBlendFactor,
	VkBlendOp colorBlendOp,
	VkBlendFactor srcAlphaBlendFactor,
	VkBlendFactor dstAlphaBlendFactor,
	VkBlendOp alphaBlendOp,
	VkColorComponentFlags colorWriteMask
)
{
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = blend;
	colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
	colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
	colorBlendAttachment.colorBlendOp = colorBlendOp;
	colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
	colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
	colorBlendAttachment.alphaBlendOp = alphaBlendOp;
	colorBlendAttachment.colorWriteMask = colorWriteMask;

	colorBlendAttachments.push_back(colorBlendAttachment);

	return *this;
}

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::addDynamicState(
	VkDynamicState state
)
{
	dynamicStates.push_back(state);

	return *this;
}

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::setInputAssemblyState(
	VkPrimitiveTopology topology,
	bool primitiveRestart
)
{
	inputAssemblyState = {};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.topology = topology;
	inputAssemblyState.primitiveRestartEnable = primitiveRestart;

	return *this;
}

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::setRasterizerState(
	bool depthClamp,
	bool rasterizerDiscard,
	VkPolygonMode polygonMode,
	float lineWidth,
	VkCullModeFlags cullMode,
	VkFrontFace frontFace,
	bool depthBias,
	float depthBiasConstantFactor,
	float depthBiasClamp,
	float depthBiasSlopeFactor
)
{
	rasterizerState = {};
	rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerState.depthClampEnable = depthClamp; // VK_TRUE value require enabling GPU feature
	rasterizerState.rasterizerDiscardEnable = rasterizerDiscard;
	rasterizerState.polygonMode = polygonMode; // other modes than VK_POLYGON_MODE_FILL require enabling GPU feature
	rasterizerState.lineWidth = lineWidth; // other values require enabling GPU feature
	rasterizerState.cullMode = cullMode;
	rasterizerState.frontFace = frontFace;
	rasterizerState.depthBiasEnable = depthBias;
	rasterizerState.depthBiasConstantFactor = depthBiasConstantFactor;
	rasterizerState.depthBiasClamp = depthBiasClamp;
	rasterizerState.depthBiasSlopeFactor = depthBiasSlopeFactor;

	return *this;
}

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::setMultisampleState(
	VkSampleCountFlagBits msaaSamples,
	bool sampleShading,
	float minSampleShading
)
{
	// Create multisampling state (enabling it requires enabling GPU feature)
	multisamplingState = {};
	multisamplingState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingState.rasterizationSamples = msaaSamples;
	multisamplingState.sampleShadingEnable = sampleShading;
	multisamplingState.minSampleShading = minSampleShading;

	multisamplingState.pSampleMask = nullptr; // Optional
	multisamplingState.alphaToCoverageEnable = VK_FALSE; // Optional
	multisamplingState.alphaToOneEnable = VK_FALSE; // Optional

	return *this;
}

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::setDepthStencilState(
	bool depthTest,
	bool depthWrite,
	VkCompareOp depthCompareOp
)
{
	// Create depth stencil state
	depthStencilState = {};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = depthTest;
	depthStencilState.depthWriteEnable = depthWrite;
	depthStencilState.depthCompareOp = depthCompareOp;

	// TODO: no stencil test at the moment
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.front = {}; // Optional
	depthStencilState.back = {}; // Optional

	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.minDepthBounds = 0.0f; // Optional
	depthStencilState.maxDepthBounds = 1.0f; // Optional

	return *this;
}

/*
 */
VkPipeline VulkanGraphicsPipelineBuilder::build(VkDevice device)
{
	VkPipelineVertexInputStateCreateInfo vertexInputState = {};
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
	vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
	vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = static_cast<uint32_t>(viewports.size());
	viewportState.pViewports = viewports.data();
	viewportState.scissorCount = static_cast<uint32_t>(scissors.size());
	viewportState.pScissors = scissors.data();

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	colorBlendState.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
	colorBlendState.pAttachments = colorBlendAttachments.data();
	colorBlendState.blendConstants[0] = 0.0f;
	colorBlendState.blendConstants[1] = 0.0f;
	colorBlendState.blendConstants[2] = 0.0f;
	colorBlendState.blendConstants[3] = 0.0f;

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	// Create graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputState;
	pipelineInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizerState;
	pipelineInfo.pMultisampleState = &multisamplingState;
	pipelineInfo.pDepthStencilState = &depthStencilState;
	pipelineInfo.pColorBlendState = &colorBlendState;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
		throw std::runtime_error("Can't create graphics pipeline");

	return pipeline;
}
