#include "render/backend/vulkan/Context.h"

#include <cassert>

namespace render::backend::vulkan
{
	void Context::pushBindSet(const BindSet *set)
	{
		assert(num_sets < MAX_SETS);
		assert(set);

		sets[num_sets++] = *set;
	}

	void Context::setBindSet(uint32_t binding, const BindSet *set)
	{
		assert(binding < num_sets);
		assert(set);

		sets[binding] = *set;
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

	void Context::clear()
	{
		clearBindSets();
		clearShaders();
		render_pass = VK_NULL_HANDLE;
		state = {};
		state.cull_mode = VK_CULL_MODE_BACK_BIT;
		state.depth_test = 1;
		state.depth_write = 1;
		state.depth_compare_func = VK_COMPARE_OP_LESS_OR_EQUAL;
	}
}
