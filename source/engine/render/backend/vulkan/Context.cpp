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
		memset(shaders, 0, sizeof(shaders));
	}

	void Context::setShader(ShaderType type, const Shader *shader)
	{
		VkShaderModule module = (shader) ? shader->module : VK_NULL_HANDLE;
 		shaders[static_cast<uint32_t>(type)] = module;
	}

	void Context::setRenderPass(const RenderPass *pass)
	{
		render_pass = VK_NULL_HANDLE;
		num_color_attachments = 0;
		samples = VK_SAMPLE_COUNT_1_BIT;

		if (!pass)
			return;

		render_pass = pass->render_pass;
		num_color_attachments = pass->num_color_attachments;
		samples = pass->max_samples;
	}

	void Context::clear()
	{
		clearBindSets();
		clearShaders();

		render_pass = VK_NULL_HANDLE;
		num_color_attachments = 0;
		samples = VK_SAMPLE_COUNT_1_BIT;

		state = {};
		state.cull_mode = VK_CULL_MODE_BACK_BIT;
		state.depth_test = 1;
		state.depth_write = 1;
		state.depth_compare_func = VK_COMPARE_OP_LESS;

		viewport = {};
		scissor = {};
	}
}
