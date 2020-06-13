#pragma once

#include <unordered_map>
#include <volk.h>

namespace render::backend
{
	struct RenderPassInfo;
}

namespace render::backend::vulkan
{
	struct FrameBuffer;
	class Device;

	/*
	 */
	class RenderPassCache
	{
	public:
		RenderPassCache(const Device *device)
			: device(device) { }
		~RenderPassCache();

		VkRenderPass fetch(const FrameBuffer *frame_buffer, const RenderPassInfo *info);
		void clear();

	private:
		uint64_t getHash(const FrameBuffer *frame_buffer, const RenderPassInfo *info) const;

	private:
		const Device *device {nullptr};
		std::unordered_map<uint64_t, VkRenderPass> cache;
	};
}
