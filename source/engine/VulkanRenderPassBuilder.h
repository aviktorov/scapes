#pragma once

#include <vector>
#include <volk.h>

/*
 */
class VulkanRenderPassBuilder
{
public:
	VulkanRenderPassBuilder() { }
	~VulkanRenderPassBuilder();

	VulkanRenderPassBuilder &VulkanRenderPassBuilder::addColorAttachment(
		VkFormat format,
		VkSampleCountFlagBits msaaSamples,
		VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
	);

	VulkanRenderPassBuilder &VulkanRenderPassBuilder::addColorResolveAttachment(
		VkFormat format,
		VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
	);

	VulkanRenderPassBuilder &addDepthStencilAttachment(
		VkFormat format,
		VkSampleCountFlagBits msaaSamples,
		VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
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

	VkRenderPass build(VkDevice device);

private:
	struct SubpassData
	{
		std::vector<VkAttachmentReference> colorAttachmentReferences;
		std::vector<VkAttachmentReference> colorAttachmentResolveReferences;
		VkAttachmentReference *depthStencilAttachmentReference {nullptr};
	};

	std::vector<VkAttachmentDescription> attachments;
	std::vector<VkSubpassDescription> subpassInfos;
	std::vector<SubpassData> subpassDatas;
};
