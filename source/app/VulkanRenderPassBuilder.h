#pragma once

#include <vector>
#include <volk.h>

#include "VulkanRendererContext.h"

/*
 */
class VulkanRenderPassBuilder
{
public:
	VulkanRenderPassBuilder(const VulkanRendererContext &context)
		: context(context) { }

	inline VkRenderPass getRenderPass() const { return renderPass; }

	VulkanRenderPassBuilder &VulkanRenderPassBuilder::addColorAttachment(
		VkFormat format,
		VkSampleCountFlagBits msaaSamples
	);

	VulkanRenderPassBuilder &VulkanRenderPassBuilder::addColorResolveAttachment(
		VkFormat format
	);

	VulkanRenderPassBuilder &addDepthStencilAttachment(
		VkFormat format,
		VkSampleCountFlagBits msaaSamples
	);

	VulkanRenderPassBuilder &addSubpass(
		VkPipelineBindPoint bindPoint
	);

	VulkanRenderPassBuilder &addColorAttachmentReference(
		int subpassIndex,
		int attachmentIndex
	);

	VulkanRenderPassBuilder &addColorResolveAttachmentReference(
		int subpassIndex,
		int attachmentIndex
	);

	VulkanRenderPassBuilder &setDepthStencilAttachmentReference(
		int subpassIndex,
		int attachmentIndex
	);

	void build();

private:
	struct SubpassData
	{
		std::vector<VkAttachmentReference> colorAttachmentReferences;
		std::vector<VkAttachmentReference> colorAttachmentResolveReferences;
		VkAttachmentReference depthStencilAttachmentReference {};
	};

	VulkanRendererContext context;

	std::vector<VkAttachmentDescription> attachments;
	std::vector<VkSubpassDescription> subpassInfos;
	std::vector<SubpassData> subpassDatas;


	VkRenderPass renderPass {VK_NULL_HANDLE};
};
