#pragma once

#include <volk.h>
#include "render/backend/vulkan/driver.h"

namespace render::backend::vulkan
{
	/*
	 */
	class Context
	{
	public:
		Context() { clear(); }
		~Context() { clear(); }

		inline void clearBindSets() { num_sets = 0; }
		inline uint8_t getNumBindSets() const { return num_sets; }

		void pushBindSet(const BindSet *set);
		void setBindSet(uint32_t binding, const BindSet *set);

		void clearShaders();

		inline VkShaderModule getShader(ShaderType type) const { return shaders[static_cast<uint32_t>(type)]; }
		void setShader(ShaderType type, const Shader *shader);

		inline VkRenderPass getRenderPass() const { return render_pass; }
		inline void setRenderPass(VkRenderPass pass) { render_pass = pass; }

		inline VkCullModeFlags getCullMode() const { return state.cull_mode; }
		inline void setCullMode(VkCullModeFlags mode) { state.cull_mode = mode; }

		inline VkCompareOp getDepthCompareFunc() const { return state.depth_compare_func; }
		inline void setDepthCompareFunc(VkCompareOp func) { state.depth_compare_func = func; }

		inline bool isDepthTestEnabled() const { return state.depth_test == 1; }
		inline void setDepthTest(bool enabled) { state.depth_test = enabled; }

		inline bool isDepthWriteEnabled() const { return state.depth_write == 1; }
		inline void setDepthWrite(bool enabled) { state.depth_write = enabled; }

		inline bool isBlendingEnabled() const { return state.blending == 1; }
		inline void setBlending(bool enabled) { state.blending = enabled; }

		inline VkBlendFactor getBlendSrcFactor() const { return state.blend_src_factor; }
		inline VkBlendFactor getBlendDstFactor() const { return state.blend_dst_factor; }
		inline void setBlendFactors(VkBlendFactor src_factor, VkBlendFactor dst_factor)
		{
			state.blend_src_factor = src_factor;
			state.blend_dst_factor = dst_factor;
		}

	private:
		void clear();

	private:
		struct RenderState
		{
			VkCullModeFlags cull_mode;
			uint8_t depth_test : 1;
			uint8_t depth_write : 1;
			VkCompareOp depth_compare_func : 3;
			uint8_t blending : 1;
			VkBlendFactor blend_src_factor;
			VkBlendFactor blend_dst_factor;
		};

	private:
		enum
		{
			MAX_SETS = 16,
		};

		BindSet sets[MAX_SETS];
		uint8_t num_sets {0};

		VkShaderModule shaders[static_cast<uint32_t>(ShaderType::MAX)];
		VkRenderPass render_pass { VK_NULL_HANDLE };

		RenderState state {};
	};
}
