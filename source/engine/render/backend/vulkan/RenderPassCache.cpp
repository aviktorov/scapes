#include "render/backend/vulkan/RenderPassCache.h"
#include "render/backend/vulkan/RenderPassBuilder.h"

#include "render/backend/vulkan/Driver.h"
#include "render/backend/vulkan/Device.h"

#include <cassert>

namespace render::backend::vulkan
{
	template <class T>
	static void hashCombine(uint64_t &s, const T &v)
	{
		std::hash<T> h;
		s^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
	}

	RenderPassCache::~RenderPassCache()
	{
		clear();
	}

	VkRenderPass RenderPassCache::fetch(const FrameBuffer *frame_buffer, const RenderPassInfo *info, VkImageLayout color_attachment_layout)
	{
		assert(info);
		assert(frame_buffer);

		uint64_t hash = getHash(frame_buffer, info);

		auto it = cache.find(hash);
		if (it != cache.end())
			return it->second;

		RenderPassBuilder builder;
		builder.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS);

		for (uint8_t i = 0; i < frame_buffer->num_color_attachments; ++i)
		{
			const FrameBufferColorAttachment &attachment = frame_buffer->color_attachments[i];

			VkFormat format = attachment.format;
			VkSampleCountFlagBits samples = attachment.samples;
			bool resolve = attachment.resolve;

			VkAttachmentLoadOp load_op = (VkAttachmentLoadOp)info->load_ops[i];
			VkAttachmentStoreOp store_op = (VkAttachmentStoreOp)info->store_ops[i];

			if(resolve)
			{
				builder.addColorResolveAttachment(format, color_attachment_layout, load_op, store_op);
				builder.addColorResolveAttachmentReference(0, i);
			}
			else
			{
				builder.addColorAttachment(format, samples, color_attachment_layout, load_op, store_op);
				builder.addColorAttachmentReference(0, i);
			}
		}

		if (frame_buffer->have_depthstencil_attachment)
		{
			const FrameBufferDepthStencilAttachment &attachment = frame_buffer->depthstencil_attachment;

			VkFormat format = attachment.format;
			VkSampleCountFlagBits samples = attachment.samples;

			// TODO: not safe, add assert
			uint8_t index = frame_buffer->num_color_attachments;

			VkAttachmentLoadOp load_op = (VkAttachmentLoadOp)info->load_ops[index];
			VkAttachmentStoreOp store_op = (VkAttachmentStoreOp)info->store_ops[index];

			VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			builder.addDepthStencilAttachment(format, samples, layout, load_op, store_op);
			builder.setDepthStencilAttachmentReference(0, index);
		}

		VkRenderPass result = builder.build(device->getDevice());
		cache[hash] = result;

		return result;
	}

	void RenderPassCache::clear()
	{
		for (auto it = cache.begin(); it != cache.end(); ++it)
			vkDestroyRenderPass(device->getDevice(), it->second, nullptr);

		cache.clear();
	}

	uint64_t RenderPassCache::getHash(const FrameBuffer *frame_buffer, const RenderPassInfo *info) const
	{
		assert(info->load_ops != nullptr);
		assert(info->store_ops != nullptr);

		uint64_t hash = 0;
		hashCombine(hash, frame_buffer->num_color_attachments);
		
		for (uint8_t i = 0; i < frame_buffer->num_color_attachments; ++i)
		{
			const FrameBufferColorAttachment &attachment = frame_buffer->color_attachments[i];

			hashCombine(hash, attachment.format);
			hashCombine(hash, attachment.samples);
			hashCombine(hash, attachment.resolve);
			hashCombine(hash, info->load_ops[i]);
			hashCombine(hash, info->store_ops[i]);
		}

		hashCombine(hash, frame_buffer->have_depthstencil_attachment);

		if (frame_buffer->have_depthstencil_attachment)
		{
			const FrameBufferDepthStencilAttachment &attachment = frame_buffer->depthstencil_attachment;

			// TODO: not safe, add assert
			uint8_t index = frame_buffer->num_color_attachments;

			VkAttachmentLoadOp load_op = (VkAttachmentLoadOp)info->load_ops[index];
			VkAttachmentStoreOp store_op = (VkAttachmentStoreOp)info->store_ops[index];

			hashCombine(hash, attachment.format);
			hashCombine(hash, attachment.samples);
			hashCombine(hash, load_op);
			hashCombine(hash, store_op);
		}

		return hash;
	}
}
