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

		inline void clearPushConstants() { push_constants_size = 0; }
		void setPushConstants(uint8_t size, const void *data);
		inline const uint8_t *getPushConstants() const { return push_constants; }
		inline uint8_t getPushConstantsSize() const { return push_constants_size; }

		inline void clearBindSets() { num_sets = 0; }
		inline uint8_t getNumBindSets() const { return num_sets; }
		inline const BindSet *getBindSet(uint8_t index) const { return sets[index]; }
		inline BindSet *getBindSet(uint8_t index) { return sets[index]; }

		void pushBindSet(BindSet *set);
		void setBindSet(uint32_t binding, BindSet *set);

		void clearShaders();

		inline VkShaderModule getShader(ShaderType type) const { return shaders[static_cast<uint32_t>(type)]; }
		void setShader(ShaderType type, const Shader *shader);

		void setFramebuffer(const FrameBuffer *frame_buffer);

		inline VkViewport getViewport() const { return viewport; }
		inline VkRect2D getScissor() const { return scissor; }

		inline VkSampleCountFlagBits getMaxSampleCount() const { return samples; }
		inline uint8_t getNumColorAttachments() const { return num_color_attachments; }

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
			MAX_PUSH_CONSTANT_SIZE = 128, // TODO: use HW device capabilities for upper limit
		};

		uint8_t push_constants[MAX_PUSH_CONSTANT_SIZE];
		uint8_t push_constants_size {0};

		BindSet *sets[MAX_SETS]; // TODO: made this safer
		uint8_t num_sets {0};

		VkViewport viewport;
		VkRect2D scissor;

		VkShaderModule shaders[static_cast<uint32_t>(ShaderType::MAX)];
		VkRenderPass render_pass { VK_NULL_HANDLE };

		VkSampleCountFlagBits samples { VK_SAMPLE_COUNT_1_BIT };
		uint8_t num_color_attachments {0};

		RenderState state {};
	};
}
