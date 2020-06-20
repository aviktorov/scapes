#include "render/backend/vulkan/PipelineCache.h"

#include "render/backend/vulkan/driver.h"
#include "render/backend/vulkan/device.h"
#include "render/backend/vulkan/context.h"
#include "render/backend/vulkan/PipelineLayoutCache.h"
#include "render/backend/vulkan/VulkanGraphicsPipelineBuilder.h"

#include <vector>
#include <cassert>

namespace render::backend::vulkan
{
	template <class T>
	static void hashCombine(uint64_t &s, const T &v)
	{
		std::hash<T> h;
		s^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
	}

	// TODO: move to utils
	static VkShaderStageFlagBits toShaderStage(ShaderType type)
	{
		static VkShaderStageFlagBits supported_stages[] =
		{
			// Graphics pipeline
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
			VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
			VK_SHADER_STAGE_GEOMETRY_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT,

			// Compute pipeline
			VK_SHADER_STAGE_COMPUTE_BIT,

			// Raytracing pipeline
			// TODO: subject of change, because Khronos announced
			// vendor independent raytracing support in Vulkan
			VK_SHADER_STAGE_RAYGEN_BIT_NV,
			VK_SHADER_STAGE_INTERSECTION_BIT_NV,
			VK_SHADER_STAGE_ANY_HIT_BIT_NV,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,
			VK_SHADER_STAGE_MISS_BIT_NV,
			VK_SHADER_STAGE_CALLABLE_BIT_NV,

		};

		return supported_stages[static_cast<int>(type)];
	};

	PipelineCache::~PipelineCache()
	{
		clear();
	}

	VkPipeline PipelineCache::fetch(const Context *context, const RenderPrimitive *primitive)
	{
		VkPipelineLayout layout = layout_cache->fetch(context);

		assert(layout != VK_NULL_HANDLE);
		assert(context->getRenderPass() != VK_NULL_HANDLE);

		uint64_t hash = getHash(layout, context, primitive);

		auto it = cache.find(hash);
		if (it != cache.end())
			return it->second;

		VulkanGraphicsPipelineBuilder builder(layout, context->getRenderPass());

		builder.addViewport(VkViewport());
		builder.addScissor(VkRect2D());
		builder.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
		builder.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);

		for (uint8_t i = 0; i < static_cast<uint8_t>(ShaderType::MAX); ++i)
		{
			VkShaderModule module = context->getShader(static_cast<ShaderType>(i));
			if (module == VK_NULL_HANDLE)
				continue;

			builder.addShaderStage(module, toShaderStage(static_cast<ShaderType>(i)));
		}

		const VertexBuffer *vertex_buffer = primitive->vertex_buffer;

		VkVertexInputBindingDescription input_binding = { 0, vertex_buffer->vertex_size, VK_VERTEX_INPUT_RATE_VERTEX };
		std::vector<VkVertexInputAttributeDescription> attributes(vertex_buffer->num_attributes);

		for (uint8_t i = 0; i < vertex_buffer->num_attributes; ++i)
			attributes[i] = { i, 0, vertex_buffer->attribute_formats[i], vertex_buffer->attribute_offsets[i] };

		builder.setInputAssemblyState(primitive->topology);
		builder.addVertexInput(input_binding, attributes);

		builder.setRasterizerState(false, false, VK_POLYGON_MODE_FILL, 1.0f, context->getCullMode(), VK_FRONT_FACE_COUNTER_CLOCKWISE);
		builder.setDepthStencilState(context->isDepthTestEnabled(), context->isDepthWriteEnabled(), context->getDepthCompareFunc());

		builder.setMultisampleState(context->getMaxSampleCount(), true);

		bool blending_enabled = context->isBlendingEnabled();
		VkBlendFactor src_factor = context->getBlendSrcFactor();
		VkBlendFactor dst_factor = context->getBlendDstFactor();

		for (uint8_t i = 0; i < context->getNumColorAttachments(); ++i)
			builder.addBlendColorAttachment(
				blending_enabled,
				src_factor,
				dst_factor,
				VK_BLEND_OP_ADD,
				src_factor,
				dst_factor);

		VkPipeline result = builder.build(device->getDevice());

		cache[hash] = result;
		return result;
	}

	void PipelineCache::clear()
	{
		for (auto it = cache.begin(); it != cache.end(); ++it)
			vkDestroyPipeline(device->getDevice(), it->second, nullptr);

		cache.clear();
	}

	uint64_t PipelineCache::getHash(VkPipelineLayout layout, const Context *context, const RenderPrimitive *primitive) const
	{
		assert(context);

		uint64_t hash = 0;
		hashCombine(hash, layout);
		hashCombine(hash, context->getRenderPass());

		const VertexBuffer *vertex_buffer = primitive->vertex_buffer;
		for (uint8_t i = 0; i < vertex_buffer->num_attributes; ++i)
		{
			hashCombine(hash, vertex_buffer->attribute_formats[i]);
			hashCombine(hash, vertex_buffer->attribute_offsets[i]);
		}

		hashCombine(hash, primitive->topology);

		for (uint8_t i = 0; i < static_cast<uint8_t>(ShaderType::MAX); ++i)
		{
			VkShaderModule module = context->getShader(static_cast<ShaderType>(i));
			if (module == VK_NULL_HANDLE)
				continue;

			hashCombine(hash, i);
			hashCombine(hash, module);
		}

		hashCombine(hash, context->getNumColorAttachments());
		hashCombine(hash, context->getMaxSampleCount());
		hashCombine(hash, context->getCullMode());
		hashCombine(hash, context->getDepthCompareFunc());
		hashCombine(hash, context->isDepthTestEnabled());
		hashCombine(hash, context->isDepthWriteEnabled());
		hashCombine(hash, context->isBlendingEnabled());
		hashCombine(hash, context->getBlendSrcFactor());
		hashCombine(hash, context->getBlendDstFactor());

		return hash;
	}
}
