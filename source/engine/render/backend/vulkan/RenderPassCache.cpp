#include "render/backend/vulkan/RenderPassCache.h"

#include "render/backend/vulkan/driver.h"
#include "render/backend/vulkan/device.h"
#include "render/backend/vulkan/VulkanRenderPassBuilder.h"

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

	VkRenderPass RenderPassCache::fetch(const FrameBuffer *frame_buffer, const RenderPassInfo *info)
	{
		uint64_t hash = getHash(frame_buffer, info);

		auto it = cache.find(hash);
		if (it != cache.end())
			return it->second;

		VulkanRenderPassBuilder builder;
		builder.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS);

		for (uint8_t i = 0; i < frame_buffer->num_attachments; ++i)
		{
			VkFormat format = frame_buffer->attachment_formats[i];
			FrameBufferAttachmentType type = frame_buffer->attachment_types[i];
			VkSampleCountFlagBits samples = frame_buffer->attachment_samples[i];
			bool resolve = frame_buffer->attachment_resolve[i];
			VkAttachmentLoadOp load_op = (VkAttachmentLoadOp)info->load_ops[i];
			VkAttachmentStoreOp store_op = (VkAttachmentStoreOp)info->store_ops[i];

			VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			if (type == FrameBufferAttachmentType::SWAP_CHAIN_COLOR)
				layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			if (type == FrameBufferAttachmentType::DEPTH)
			{
				builder.addDepthStencilAttachment(format, samples, layout, load_op, store_op);
				builder.setDepthStencilAttachmentReference(0, i);
			}
			else if(resolve)
			{
				builder.addColorResolveAttachment(format, layout, load_op, store_op);
				builder.addColorResolveAttachmentReference(0, i);
			}
			else
			{
				builder.addColorAttachment(format, samples, layout, load_op, store_op);
				builder.addColorAttachmentReference(0, i);
			}
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
		hashCombine(hash, frame_buffer->num_attachments);
		
		for (uint8_t i = 0; i < frame_buffer->num_attachments; ++i)
		{
			hashCombine(hash, frame_buffer->attachment_formats[i]);
			hashCombine(hash, frame_buffer->attachment_samples[i]);
			hashCombine(hash, frame_buffer->attachment_resolve[i]);
			hashCombine(hash, info->load_ops[i]);
			hashCombine(hash, info->store_ops[i]);
		}

		return hash;
	}
}