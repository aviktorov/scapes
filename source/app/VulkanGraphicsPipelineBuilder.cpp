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

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::addDescriptorSetLayoutBinding(
	uint32_t binding,
	VkDescriptorType type,
	VkShaderStageFlags shaderStageFlags
)
{
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
	descriptorSetLayoutBinding.binding = binding;
	descriptorSetLayoutBinding.descriptorType = type;
	descriptorSetLayoutBinding.descriptorCount = 1;
	descriptorSetLayoutBinding.stageFlags = shaderStageFlags;

	descriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);

	return *this;
}

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::addColorAttachment(
	VkFormat format,
	VkSampleCountFlagBits msaaSamples
)
{
	// Create render pass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = format;
	colorAttachment.samples = msaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	renderPassAttachments.push_back(colorAttachment);

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = static_cast<uint32_t>(renderPassAttachments.size() - 1);
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	if (msaaSamples == VK_SAMPLE_COUNT_1_BIT)
		return *this;

	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = format;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	renderPassAttachments.push_back(colorAttachmentResolve);

	VkAttachmentReference colorAttachmentResolveReference = {};
	colorAttachmentResolveReference.attachment = static_cast<uint32_t>(renderPassAttachments.size() - 1);
	colorAttachmentResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	return *this;
}

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::addDepthStencilAttachment(
	VkFormat format,
	VkSampleCountFlagBits msaaSamples
)
{
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = format;
	depthAttachment.samples = msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	renderPassAttachments.push_back(depthAttachment);

	depthAttachmentReference = {};
	depthAttachmentReference.attachment = static_cast<uint32_t>(renderPassAttachments.size() - 1);
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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
void VulkanGraphicsPipelineBuilder::build()
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

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
	descriptorSetLayoutInfo.pBindings = descriptorSetLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(context.device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Can't create descriptor set layout");

	// Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // TODO: add support for push constants
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Can't create pipeline layout");

	VkSubpassDescription subpassInfo = {};
	subpassInfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassInfo.pDepthStencilAttachment = &depthAttachmentReference;
	subpassInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentReferences.size());
	subpassInfo.pColorAttachments = colorAttachmentReferences.data();
	subpassInfo.pResolveAttachments = colorAttachmentResolveReferences.data();

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
	renderPassInfo.pAttachments = renderPassAttachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassInfo;

	if (vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		throw std::runtime_error("Can't create render pass");

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
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
		throw std::runtime_error("Can't create graphics pipeline");
}
