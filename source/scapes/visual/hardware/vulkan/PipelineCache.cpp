#include <hardware/vulkan/PipelineCache.h>
#include <hardware/vulkan/PipelineLayoutCache.h>
#include <hardware/vulkan/GraphicsPipelineBuilder.h>

#include <hardware/vulkan/Device.h>
#include <hardware/vulkan/Context.h>
#include <hardware/vulkan/Utils.h>

#include <HashUtils.h>

#include <vector>
#include <cassert>

namespace scapes::visual::hardware::vulkan
{
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
			VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
			VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
			VK_SHADER_STAGE_MISS_BIT_KHR,
			VK_SHADER_STAGE_CALLABLE_BIT_KHR,

		};

		return supported_stages[static_cast<int>(type)];
	};

	PipelineCache::~PipelineCache()
	{
		clear();
	}

	VkPipeline PipelineCache::fetch(VkPipelineLayout layout, const GraphicsPipeline *graphics_pipeline)
	{
		assert(layout != VK_NULL_HANDLE);
		assert(graphics_pipeline);
		assert(graphics_pipeline->render_pass != VK_NULL_HANDLE);

		uint64_t hash = getHash(layout, graphics_pipeline);

		auto it = graphics_pipeline_cache.find(hash);
		if (it != graphics_pipeline_cache.end())
			return it->second;

		GraphicsPipelineBuilder builder(layout, graphics_pipeline->render_pass);

		builder.addViewport(VkViewport());
		builder.addScissor(VkRect2D());
		builder.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
		builder.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);

		for (uint8_t i = 0; i < static_cast<uint8_t>(ShaderType::MAX); ++i)
		{
			VkShaderModule module = graphics_pipeline->shaders[i];
			if (module == VK_NULL_HANDLE)
				continue;

			builder.addShaderStage(module, toShaderStage(static_cast<ShaderType>(i)));
		}

		uint32_t attribute_location = 0;
		for (uint8_t i = 0; i < graphics_pipeline->num_vertex_streams; ++i)
		{
			const VertexBuffer *vertex_buffer = graphics_pipeline->vertex_streams[i];

			VkVertexInputBindingDescription input_binding = { i, vertex_buffer->vertex_size, VK_VERTEX_INPUT_RATE_VERTEX };
			std::vector<VkVertexInputAttributeDescription> attributes(vertex_buffer->num_attributes);

			for (uint8_t j = 0; j < vertex_buffer->num_attributes; ++j)
				attributes[j] = { attribute_location++, i, vertex_buffer->attribute_formats[j], vertex_buffer->attribute_offsets[j] };

			builder.addVertexInput(input_binding, attributes);
		}

		builder.setInputAssemblyState(graphics_pipeline->primitive_topology);

		builder.setRasterizerState(false, false, VK_POLYGON_MODE_FILL, 1.0f, graphics_pipeline->cull_mode, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		builder.setDepthStencilState(graphics_pipeline->depth_test, graphics_pipeline->depth_write, graphics_pipeline->depth_compare_func);

		builder.setMultisampleState(graphics_pipeline->max_samples, true);

		bool blending_enabled = graphics_pipeline->blending;
		VkBlendFactor src_factor = graphics_pipeline->blend_src_factor;
		VkBlendFactor dst_factor = graphics_pipeline->blend_dst_factor;

		for (uint8_t i = 0; i < graphics_pipeline->num_color_attachments; ++i)
			builder.addBlendColorAttachment(
				blending_enabled,
				src_factor,
				dst_factor,
				VK_BLEND_OP_ADD,
				src_factor,
				dst_factor);

		VkPipeline result = builder.build(context->getDevice());

		graphics_pipeline_cache[hash] = result;
		return result;
	}

	VkPipeline PipelineCache::fetch(VkPipelineLayout layout, const RayTracePipeline *raytrace_pipeline)
	{
		assert(layout != VK_NULL_HANDLE);
		assert(raytrace_pipeline);

		uint64_t hash = getHash(layout, raytrace_pipeline);

		auto it = raytrace_pipeline_cache.find(hash);
		if (it != raytrace_pipeline_cache.end())
			return it->second;

		constexpr uint32_t max_shaders = RayTracePipeline::MAX_RAYGEN_SHADERS + RayTracePipeline::MAX_MISS_SHADERS + RayTracePipeline::MAX_HITGROUP_SHADERS * 3;
		constexpr uint32_t max_groups = RayTracePipeline::MAX_RAYGEN_SHADERS + RayTracePipeline::MAX_MISS_SHADERS + RayTracePipeline::MAX_HITGROUP_SHADERS;

		VkPipelineShaderStageCreateInfo shader_stages[max_shaders];
		VkRayTracingShaderGroupCreateInfoKHR  shader_groups[max_groups];

		uint32_t num_shaders = 0;
		uint32_t num_groups = 0;

		for (uint32_t i = 0; i < raytrace_pipeline->num_raygen_shaders; ++i)
		{
			VkPipelineShaderStageCreateInfo &stage = shader_stages[num_shaders];
			VkRayTracingShaderGroupCreateInfoKHR &group = shader_groups[num_groups];

			stage = {};
			stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			stage.module = raytrace_pipeline->raygen_shaders[i];
			stage.pName = "main";

			group = {};
			group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			group.generalShader = num_shaders;
			group.closestHitShader = VK_SHADER_UNUSED_KHR;
			group.anyHitShader = VK_SHADER_UNUSED_KHR;
			group.intersectionShader = VK_SHADER_UNUSED_KHR;

			num_shaders++;
			num_groups++;
		}

		for (uint32_t i = 0; i < raytrace_pipeline->num_miss_shaders; ++i)
		{
			VkPipelineShaderStageCreateInfo &stage = shader_stages[num_shaders];
			VkRayTracingShaderGroupCreateInfoKHR &group = shader_groups[num_groups];

			stage = {};
			stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
			stage.module = raytrace_pipeline->miss_shaders[i];
			stage.pName = "main";

			group = {};
			group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			group.generalShader = num_shaders;
			group.closestHitShader = VK_SHADER_UNUSED_KHR;
			group.anyHitShader = VK_SHADER_UNUSED_KHR;
			group.intersectionShader = VK_SHADER_UNUSED_KHR;

			num_shaders++;
			num_groups++;
		}

		for (uint32_t i = 0; i < raytrace_pipeline->num_hitgroup_shaders; ++i)
		{
			VkRayTracingShaderGroupCreateInfoKHR &group = shader_groups[num_groups];

			group = {};
			group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			group.generalShader = VK_SHADER_UNUSED_KHR;
			group.closestHitShader = VK_SHADER_UNUSED_KHR;
			group.anyHitShader = VK_SHADER_UNUSED_KHR;
			group.intersectionShader = VK_SHADER_UNUSED_KHR;

			if (raytrace_pipeline->intersection_shaders[i])
			{
				VkPipelineShaderStageCreateInfo &stage = shader_stages[num_shaders];

				stage = {};
				stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stage.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				stage.module = raytrace_pipeline->intersection_shaders[i];
				stage.pName = "main";

				group.intersectionShader = num_shaders;

				num_shaders++;
			}

			if (raytrace_pipeline->anyhit_shaders[i])
			{
				VkPipelineShaderStageCreateInfo &stage = shader_stages[num_shaders];

				stage = {};
				stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				stage.module = raytrace_pipeline->anyhit_shaders[i];
				stage.pName = "main";

				group.anyHitShader = num_shaders;

				num_shaders++;
			}

			if (raytrace_pipeline->closesthit_shaders[i])
			{
				VkPipelineShaderStageCreateInfo &stage = shader_stages[num_shaders];

				stage = {};
				stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				stage.module = raytrace_pipeline->closesthit_shaders[i];
				stage.pName = "main";

				group.closestHitShader = num_shaders;

				num_shaders++;
			}

			num_groups++;
		}

		// TODO: callable shaders

		VkRayTracingPipelineCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		create_info.stageCount = num_shaders;
		create_info.pStages = shader_stages;
		create_info.groupCount = num_groups;
		create_info.pGroups = shader_groups;
		create_info.maxPipelineRayRecursionDepth = 1; // TODO: could be potentially set outside?
		create_info.layout = layout;

		VkPipeline result = VK_NULL_HANDLE;
		if (vkCreateRayTracingPipelinesKHR(context->getDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &create_info, nullptr, &result) != VK_SUCCESS)
		{
			// TODO: log error
		}

		raytrace_pipeline_cache[hash] = result;
		return result;
	}

	void PipelineCache::clear()
	{
		for (auto it = graphics_pipeline_cache.begin(); it != graphics_pipeline_cache.end(); ++it)
			vkDestroyPipeline(context->getDevice(), it->second, nullptr);

		for (auto it = raytrace_pipeline_cache.begin(); it != raytrace_pipeline_cache.end(); ++it)
			vkDestroyPipeline(context->getDevice(), it->second, nullptr);

		graphics_pipeline_cache.clear();
		raytrace_pipeline_cache.clear();
	}

	uint64_t PipelineCache::getHash(VkPipelineLayout layout, const GraphicsPipeline *graphics_pipeline) const
	{
		assert(graphics_pipeline);

		uint64_t hash = 0;
		common::HashUtils::combine(hash, layout);
		common::HashUtils::combine(hash, graphics_pipeline->render_pass);

		for (uint8_t i = 0; i < graphics_pipeline->num_vertex_streams; ++i)
		{
			const VertexBuffer *vertex_buffer = graphics_pipeline->vertex_streams[i];

			for (uint8_t j = 0; j < vertex_buffer->num_attributes; ++j)
			{
				common::HashUtils::combine(hash, vertex_buffer->attribute_formats[j]);
				common::HashUtils::combine(hash, vertex_buffer->attribute_offsets[j]);
			}
		}

		common::HashUtils::combine(hash, graphics_pipeline->primitive_topology);

		for (uint8_t i = 0; i < static_cast<uint8_t>(ShaderType::MAX); ++i)
		{
			VkShaderModule module = graphics_pipeline->shaders[i];
			if (module == VK_NULL_HANDLE)
				continue;

			common::HashUtils::combine(hash, i);
			common::HashUtils::combine(hash, module);
		}

		common::HashUtils::combine(hash, graphics_pipeline->num_color_attachments);
		common::HashUtils::combine(hash, graphics_pipeline->max_samples);
		common::HashUtils::combine(hash, graphics_pipeline->cull_mode);
		common::HashUtils::combine(hash, graphics_pipeline->depth_compare_func);
		common::HashUtils::combine(hash, graphics_pipeline->depth_test);
		common::HashUtils::combine(hash, graphics_pipeline->depth_write);
		common::HashUtils::combine(hash, graphics_pipeline->blending);
		common::HashUtils::combine(hash, graphics_pipeline->blend_src_factor);
		common::HashUtils::combine(hash, graphics_pipeline->blend_dst_factor);

		return hash;
	}

	uint64_t PipelineCache::getHash(VkPipelineLayout layout, const RayTracePipeline *raytrace_pipeline) const
	{
		assert(raytrace_pipeline);

		uint64_t hash = 0;
		common::HashUtils::combine(hash, layout);

		for (uint8_t i = 0; i < raytrace_pipeline->num_raygen_shaders; ++i)
			common::HashUtils::combine(hash, raytrace_pipeline->raygen_shaders[i]);

		for (uint8_t i = 0; i < raytrace_pipeline->num_miss_shaders; ++i)
			common::HashUtils::combine(hash, raytrace_pipeline->miss_shaders[i]);

		for (uint8_t i = 0; i < raytrace_pipeline->num_hitgroup_shaders; ++i)
		{
			common::HashUtils::combine(hash, raytrace_pipeline->intersection_shaders[i]);
			common::HashUtils::combine(hash, raytrace_pipeline->anyhit_shaders[i]);
			common::HashUtils::combine(hash, raytrace_pipeline->closesthit_shaders[i]);
		}

		return hash;
	}
}
