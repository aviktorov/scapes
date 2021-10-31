#include "render/vulkan/RenderPassBuilder.h"

namespace scapes::foundation::render::vulkan
{
	/*
	 */
	RenderPassBuilder::~RenderPassBuilder()
	{
		for (SubpassData &data : subpass_datas)
			delete data.depth_stencil_attachment_reference;
	}

	/*
	 */
	RenderPassBuilder &RenderPassBuilder::addAttachment(
		VkFormat format,
		VkSampleCountFlagBits num_samples,
		VkAttachmentLoadOp load_op,
		VkAttachmentStoreOp store_op,
		VkAttachmentLoadOp stencil_load_op,
		VkAttachmentStoreOp stencil_store_op,
		VkImageLayout final_layout
	)
	{
		VkAttachmentDescription attachment = {};
		attachment.format = format;
		attachment.samples = num_samples;
		attachment.loadOp = load_op;
		attachment.storeOp = store_op;
		attachment.stencilLoadOp = stencil_load_op;
		attachment.stencilStoreOp = stencil_store_op;
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout = final_layout;

		attachments.push_back(attachment);

		return *this;
	}

	/*
	 */
	RenderPassBuilder &RenderPassBuilder::addSubpass(
		VkPipelineBindPoint bind_point
	)
	{
		VkSubpassDescription info = {};
		info.pipelineBindPoint = bind_point;

		subpass_infos.push_back(info);
		subpass_datas.push_back(SubpassData());

		return *this;
	}

	RenderPassBuilder &RenderPassBuilder::addColorAttachmentReference(
		int subpass,
		int attachment
	)
	{
		if (subpass < 0 || subpass >= subpass_infos.size())
			return *this;

		if (attachment < 0 || attachment >= attachments.size())
			return *this;

		VkAttachmentReference reference = {};
		reference.attachment = attachment;
		reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		SubpassData &data = subpass_datas[subpass];
		data.color_attachment_references.push_back(reference);

		return *this;
	}

	RenderPassBuilder &RenderPassBuilder::addColorResolveAttachmentReference(
		int subpass,
		int attachment
	)
	{
		if (subpass < 0 || subpass >= subpass_infos.size())
			return *this;

		if (attachment < 0 || attachment >= attachments.size())
			return *this;

		VkAttachmentReference reference = {};
		reference.attachment = attachment;
		reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		SubpassData &data = subpass_datas[subpass];
		data.color_attachment_resolve_references.push_back(reference);

		return *this;
	}

	RenderPassBuilder &RenderPassBuilder::setDepthStencilAttachmentReference(
		int subpass,
		int attachment
	)
	{
		if (subpass < 0 || subpass >= subpass_infos.size())
			return *this;

		if (attachment < 0 || attachment >= attachments.size())
			return *this;

		SubpassData &data = subpass_datas[subpass];

		if (data.depth_stencil_attachment_reference == nullptr)
			data.depth_stencil_attachment_reference = new VkAttachmentReference();

		VkAttachmentReference reference = {};
		reference.attachment = attachment;
		reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		*(data.depth_stencil_attachment_reference) = reference;

		return *this;
	}

	/*
	 */
	VkRenderPass RenderPassBuilder::build(VkDevice device)
	{
		for (int i = 0; i < subpass_infos.size(); i++)
		{
			SubpassData &data = subpass_datas[i];
			VkSubpassDescription &info = subpass_infos[i];

			info.pDepthStencilAttachment = data.depth_stencil_attachment_reference;
			info.colorAttachmentCount = static_cast<uint32_t>(data.color_attachment_references.size());
			info.pColorAttachments = data.color_attachment_references.data();
			info.pResolveAttachments = data.color_attachment_resolve_references.data();
		}

		VkRenderPassCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.attachmentCount = static_cast<uint32_t>(attachments.size());
		info.pAttachments = attachments.data();
		info.subpassCount = static_cast<uint32_t>(subpass_infos.size());
		info.pSubpasses = subpass_infos.data();

		VkRenderPass result = VK_NULL_HANDLE;
		if (vkCreateRenderPass(device, &info, nullptr, &result) != VK_SUCCESS)
		{
			// TODO: log error "Can't create render pass"
		}

		return result;
	}
}
