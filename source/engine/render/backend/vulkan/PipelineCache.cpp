#include "render/backend/vulkan/PipelineCache.h"
#include "render/backend/vulkan/PipelineLayoutCache.h"
#include "render/backend/vulkan/GraphicsPipelineBuilder.h"

#include "render/backend/vulkan/Driver.h"
#include "render/backend/vulkan/Device.h"
#include "render/backend/vulkan/Utils.h"

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
		static VkShaderStageFlagBits supported_stages[12] =
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

	VkPipeline PipelineCache::fetch(VkPipelineLayout layout, const PipelineState *pipeline_state)
	{
		assert(layout != VK_NULL_HANDLE);
		assert(pipeline_state);
		assert(pipeline_state->render_pass != VK_NULL_HANDLE);

		uint64_t hash = getHash(layout, pipeline_state);

		auto it = cache.find(hash);
		if (it != cache.end())
			return it->second;

		GraphicsPipelineBuilder builder(layout, pipeline_state->render_pass);

		builder.addViewport(VkViewport());
		builder.addScissor(VkRect2D());
		builder.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
		builder.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);

		for (uint8_t i = 0; i < static_cast<uint8_t>(ShaderType::MAX); ++i)
		{
			VkShaderModule module = pipeline_state->shaders[i];
			if (module == VK_NULL_HANDLE)
				continue;

			builder.addShaderStage(module, toShaderStage(static_cast<ShaderType>(i)));
		}

		uint32_t attribute_location = 0;
		for (uint8_t i = 0; i < pipeline_state->num_vertex_streams; ++i)
		{
			const VertexBuffer *vertex_buffer = pipeline_state->vertex_streams[i];

			VkVertexInputBindingDescription input_binding = { i, vertex_buffer->vertex_size, VK_VERTEX_INPUT_RATE_VERTEX };
			std::vector<VkVertexInputAttributeDescription> attributes(vertex_buffer->num_attributes);

			for (uint8_t j = 0; j < vertex_buffer->num_attributes; ++j)
				attributes[j] = { attribute_location++, i, vertex_buffer->attribute_formats[j], vertex_buffer->attribute_offsets[j] };

			builder.addVertexInput(input_binding, attributes);
		}

		builder.setInputAssemblyState(pipeline_state->primitive_topology);

		builder.setRasterizerState(false, false, VK_POLYGON_MODE_FILL, 1.0f, pipeline_state->cull_mode, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		builder.setDepthStencilState(pipeline_state->depth_test, pipeline_state->depth_write, pipeline_state->depth_compare_func);

		builder.setMultisampleState(pipeline_state->max_samples, true);

		bool blending_enabled = pipeline_state->blending;
		VkBlendFactor src_factor = pipeline_state->blend_src_factor;
		VkBlendFactor dst_factor = pipeline_state->blend_dst_factor;

		for (uint8_t i = 0; i < pipeline_state->num_color_attachments; ++i)
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

	uint64_t PipelineCache::getHash(VkPipelineLayout layout, const PipelineState *pipeline_state) const
	{
		assert(pipeline_state);

		uint64_t hash = 0;
		hashCombine(hash, layout);
		hashCombine(hash, pipeline_state->render_pass);

		for (uint8_t i = 0; i < pipeline_state->num_vertex_streams; ++i)
		{
			const VertexBuffer *vertex_buffer = pipeline_state->vertex_streams[i];

			for (uint8_t j = 0; j < vertex_buffer->num_attributes; ++j)
			{
				hashCombine(hash, vertex_buffer->attribute_formats[j]);
				hashCombine(hash, vertex_buffer->attribute_offsets[j]);

			}
		}

		hashCombine(hash, pipeline_state->primitive_topology);

		for (uint8_t i = 0; i < static_cast<uint8_t>(ShaderType::MAX); ++i)
		{
			VkShaderModule module = pipeline_state->shaders[i];
			if (module == VK_NULL_HANDLE)
				continue;

			hashCombine(hash, i);
			hashCombine(hash, module);
		}

		hashCombine(hash, pipeline_state->num_color_attachments);
		hashCombine(hash, pipeline_state->max_samples);
		hashCombine(hash, pipeline_state->cull_mode);
		hashCombine(hash, pipeline_state->depth_compare_func);
		hashCombine(hash, pipeline_state->depth_test);
		hashCombine(hash, pipeline_state->depth_write);
		hashCombine(hash, pipeline_state->blending);
		hashCombine(hash, pipeline_state->blend_src_factor);
		hashCombine(hash, pipeline_state->blend_dst_factor);

		return hash;
	}
}
