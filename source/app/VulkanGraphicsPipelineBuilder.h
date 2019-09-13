#pragma once

#include <vector>
#include <volk.h>

#include "VulkanRendererContext.h"

/*
 */
class VulkanGraphicsPipelineBuilder
{
public:
	VulkanGraphicsPipelineBuilder(const VulkanRendererContext &context)
		: context(context) { }

	inline VkRenderPass getRenderPass() const { return renderPass; }
	inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
	inline VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
	inline VkPipeline getPipeline() const { return pipeline; }

	VulkanGraphicsPipelineBuilder &addShaderStage(
		VkShaderModule shader,
		VkShaderStageFlagBits stage,
		const char *entry = "main"
	);

	VulkanGraphicsPipelineBuilder &addVertexInput(
		const VkVertexInputBindingDescription &binding,
		const std::vector<VkVertexInputAttributeDescription> &attributes
	);

	VulkanGraphicsPipelineBuilder &addViewport(
		const VkViewport &viewport
	);

	VulkanGraphicsPipelineBuilder &addScissor(
		const VkRect2D &scissor
	);


	VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::addBlendColorAttachment(
		bool blend = false,
		VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
		VkBlendOp colorBlendOp = VK_BLEND_OP_ADD,
		VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD,
		VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	);

	VulkanGraphicsPipelineBuilder &addDescriptorSetLayoutBinding(
		uint32_t binding,
		VkDescriptorType type,
		VkShaderStageFlags shaderStageFlags
	);

	VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::addColorAttachment(
		VkFormat format,
		VkSampleCountFlagBits msaaSamples
	);

	VulkanGraphicsPipelineBuilder &addDepthStencilAttachment(
		VkFormat format,
		VkSampleCountFlagBits msaaSamples
	);

	VulkanGraphicsPipelineBuilder &setInputAssemblyState(
		VkPrimitiveTopology topology,
		bool primitiveRestart = false
	);

	VulkanGraphicsPipelineBuilder &setRasterizerState(
		bool depthClamp,
		bool rasterizerDiscard,
		VkPolygonMode polygonMode,
		float lineWidth,
		VkCullModeFlags cullMode,
		VkFrontFace frontFace,
		bool depthBias = false,
		float depthBiasConstantFactor = 0.0f,
		float depthBiasClamp = 0.0f,
		float depthBiasSlopeFactor = 0.0f
	);

	VulkanGraphicsPipelineBuilder &setMultisampleState(
		VkSampleCountFlagBits msaaSamples,
		bool sampleShading,
		float minSampleShading
	);

	VulkanGraphicsPipelineBuilder &setDepthStencilState(
		bool depthTest,
		bool depthWrite,
		VkCompareOp depthCompareOp
	);

	void build();

private:
	VulkanRendererContext context;

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	std::vector<VkVertexInputBindingDescription> vertexInputBindings;
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;

	std::vector<VkViewport> viewports;
	std::vector<VkRect2D> scissors;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState {};
	VkPipelineRasterizationStateCreateInfo rasterizerState {};
	VkPipelineMultisampleStateCreateInfo multisamplingState {};
	VkPipelineDepthStencilStateCreateInfo depthStencilState {};

	std::vector<VkAttachmentDescription> renderPassAttachments;

	std::vector<VkAttachmentReference> colorAttachmentReferences;
	std::vector<VkAttachmentReference> colorAttachmentResolveReferences;
	VkAttachmentReference depthAttachmentReference {};

	VkRenderPass renderPass {VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptorSetLayout {VK_NULL_HANDLE};
	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
	VkPipeline pipeline {VK_NULL_HANDLE};
};
