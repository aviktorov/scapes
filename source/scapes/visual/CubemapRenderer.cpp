#include "CubemapRenderer.h"

#include <scapes/visual/Resources.h>
#include <scapes/foundation/math/Math.h>

#include <algorithm>

using namespace scapes::foundation;

namespace scapes::visual
{
	/*
	 */
	struct CubemapFaceOrientationData
	{
		math::mat4 faces[6];
	};

	/*
	 */
	CubemapRenderer::CubemapRenderer(render::Device *device)
		: device(device)
	{
	}

	CubemapRenderer::~CubemapRenderer()
	{
		shutdown();
	}

	/*
	 */
	void CubemapRenderer::init(
		const resources::Texture *target_texture,
		uint32_t target_mip
	)
	{
		// Create uniform buffers
		uint32_t ubo_size = sizeof(CubemapFaceOrientationData);
		uniform_buffer = device->createUniformBuffer(render::BufferType::DYNAMIC, ubo_size);

		// Create bind set
		bind_set = device->createBindSet();

		// Create framebuffer
		render::FrameBufferAttachment frame_buffer_attachments[6] =
		{
			{ target_texture->gpu_data, target_mip, 0, 1 },
			{ target_texture->gpu_data, target_mip, 1, 1 },
			{ target_texture->gpu_data, target_mip, 2, 1 },
			{ target_texture->gpu_data, target_mip, 3, 1 },
			{ target_texture->gpu_data, target_mip, 4, 1 },
			{ target_texture->gpu_data, target_mip, 5, 1 },
		};

		frame_buffer = device->createFrameBuffer(6, frame_buffer_attachments);

		// Create render pass
		render::RenderPassClearValue clear_value;
		clear_value.as_color = { 0.0f, 0.0f, 0.0f, 1.0f };

		render::Multisample samples = render::Multisample::COUNT_1;
		render::RenderPassLoadOp load_op = render::RenderPassLoadOp::CLEAR;
		render::RenderPassStoreOp store_op = render::RenderPassStoreOp::STORE;

		render::RenderPassAttachment render_pass_attachments[6] =
		{
			{ target_texture->format, samples, load_op, store_op, clear_value },
			{ target_texture->format, samples, load_op, store_op, clear_value },
			{ target_texture->format, samples, load_op, store_op, clear_value },
			{ target_texture->format, samples, load_op, store_op, clear_value },
			{ target_texture->format, samples, load_op, store_op, clear_value },
			{ target_texture->format, samples, load_op, store_op, clear_value },
		};

		uint32_t color_attachments[6] = { 0, 1, 2, 3, 4, 5 };

		render::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 6;
		render_pass_description.color_attachments = color_attachments;

		render_pass = device->createRenderPass(6, render_pass_attachments, render_pass_description);

		// Create command buffer
		command_buffer = device->createCommandBuffer(render::CommandBufferType::PRIMARY);

		// Create pipeline state
		uint32_t target_width = std::max<uint32_t>(1, target_texture->width << target_mip);
		uint32_t target_height = std::max<uint32_t>(1, target_texture->height << target_mip);

		pipeline_state = device->createPipelineState();
		device->setViewport(pipeline_state, 0, 0, target_width, target_height);
		device->setScissor(pipeline_state, 0, 0, target_width, target_height);

		// Fill uniform buffer
		CubemapFaceOrientationData *ubo = reinterpret_cast<CubemapFaceOrientationData *>(device->map(uniform_buffer));

		const math::mat4 &translateZ = math::translate(math::mat4(1.0f), math::vec3(0.0f, 0.0f, 1.0f));

		const math::vec3 faceDirs[6] =
		{
			math::vec3( 1.0f,  0.0f,  0.0f),
			math::vec3(-1.0f,  0.0f,  0.0f),
			math::vec3( 0.0f,  1.0f,  0.0f),
			math::vec3( 0.0f, -1.0f,  0.0f),
			math::vec3( 0.0f,  0.0f,  1.0f),
			math::vec3( 0.0f,  0.0f, -1.0f),
		};

		const math::vec3 faceUps[6] = {
			math::vec3( 0.0f,  0.0f, -1.0f),
			math::vec3( 0.0f,  0.0f,  1.0f),
			math::vec3(-1.0f,  0.0f,  0.0f),
			math::vec3(-1.0f,  0.0f,  0.0f),
			math::vec3( 0.0f, -1.0f,  0.0f),
			math::vec3( 0.0f, -1.0f,  0.0f),
		};

		// TODO: get rid of this mess
		const math::mat4 faceRotations[6] = {
			math::rotate(math::mat4(1.0f), math::radians(180.0f), math::vec3(0.0f, 1.0f, 0.0f)),
			math::rotate(math::mat4(1.0f), math::radians(180.0f), math::vec3(0.0f, 1.0f, 0.0f)),
			math::mat4(1.0f),
			math::mat4(1.0f),
			math::rotate(math::mat4(1.0f), math::radians(-90.0f), math::vec3(0.0f, 0.0f, 1.0f)),
			math::rotate(math::mat4(1.0f), math::radians(-90.0f), math::vec3(0.0f, 0.0f, 1.0f)),
		};

		for (int i = 0; i < 6; i++)
			ubo->faces[i] = faceRotations[i] * math::lookAtRH(math::vec3(0.0f), faceDirs[i], faceUps[i]) * translateZ;

		device->unmap(uniform_buffer);

		// Bind data to descriptor set
		device->bindUniformBuffer(bind_set, 0, uniform_buffer);
	}

	void CubemapRenderer::shutdown()
	{
		device->destroyUniformBuffer(uniform_buffer);
		uniform_buffer = nullptr;

		device->destroyFrameBuffer(frame_buffer);
		frame_buffer = nullptr;

		device->destroyRenderPass(render_pass);
		render_pass = nullptr;

		device->destroyCommandBuffer(command_buffer);
		command_buffer = nullptr;

		device->destroyBindSet(bind_set);
		bind_set = nullptr;

		device->destroyPipelineState(pipeline_state);
		pipeline_state = nullptr;
	}

	/*
	 */
	void CubemapRenderer::render(
		const resources::Mesh *mesh,
		const resources::Shader *vertex_shader,
		const resources::Shader *fragment_shader,
		const resources::Texture *input_texture,
		uint8_t push_constants_size,
		const uint8_t *push_constants_data
	)
	{
		device->bindTexture(bind_set, 1, input_texture->gpu_data);

		device->setPushConstants(pipeline_state, push_constants_size, push_constants_data);
		device->setBindSet(pipeline_state, 0, bind_set);
		device->setShader(pipeline_state, render::ShaderType::VERTEX, vertex_shader->shader);
		device->setShader(pipeline_state, render::ShaderType::FRAGMENT, fragment_shader->shader);

		device->setVertexStream(pipeline_state, 0, mesh->vertex_buffer);

		device->resetCommandBuffer(command_buffer);
		device->beginCommandBuffer(command_buffer);
		device->beginRenderPass(command_buffer, render_pass, frame_buffer);

		device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, mesh->index_buffer, mesh->num_indices);

		device->endRenderPass(command_buffer);
		device->endCommandBuffer(command_buffer);

		device->submit(command_buffer);
		device->wait(1, &command_buffer);
	}
}
