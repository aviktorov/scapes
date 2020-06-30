#include "render/backend/vulkan/Context.h"

#include <algorithm>
#include <cassert>

namespace render::backend::vulkan
{
	void Context::setPushConstants(uint8_t size, const void *data)
	{
		assert(size < MAX_PUSH_CONSTANT_SIZE);

		memcpy(push_constants, data, size);
		push_constants_size = size;
	}

	void Context::pushBindSet(BindSet *set)
	{
		assert(num_sets < MAX_SETS);
		assert(set);

		sets[num_sets++] = set;
	}

	void Context::setBindSet(uint32_t binding, BindSet *set)
	{
		assert(binding < num_sets);
		assert(set);

		sets[binding] = set;
	}

	void Context::clearShaders()
	{
		memset(shaders, VK_NULL_HANDLE, sizeof(shaders));
	}

	void Context::setShader(ShaderType type, const Shader *shader)
	{
		VkShaderModule module = (shader) ? shader->module : VK_NULL_HANDLE;
 		shaders[static_cast<uint32_t>(type)] = module;
	}

	void Context::setFramebuffer(const FrameBuffer *frame_buffer)
	{
		samples = VK_SAMPLE_COUNT_1_BIT;
		num_color_attachments = 0;

		if (frame_buffer == nullptr)
			return;

		for (uint8_t i = 0; i < frame_buffer->num_attachments; ++i)
		{
			if (frame_buffer->attachment_types[i] == FrameBufferAttachmentType::DEPTH)
				continue;
			
			if (frame_buffer->attachment_resolve[i])
				continue;
			
			num_color_attachments++;
			samples = std::max(samples, frame_buffer->attachment_samples[i]);
		}

		viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(frame_buffer->sizes.width);
		viewport.height = static_cast<float>(frame_buffer->sizes.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		scissor = {};
		scissor.offset = {0, 0};
		scissor.extent = frame_buffer->sizes;
	}

	void Context::clear()
	{
		clearBindSets();
		clearShaders();
		render_pass = VK_NULL_HANDLE;
		state = {};
		state.cull_mode = VK_CULL_MODE_BACK_BIT;
		state.depth_test = 1;
		state.depth_write = 1;
		state.depth_compare_func = VK_COMPARE_OP_LESS;

		viewport = {};
		scissor = {};
	}
}
