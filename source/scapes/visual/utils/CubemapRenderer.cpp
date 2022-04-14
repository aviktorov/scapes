#include "CubemapRenderer.h"

#include <scapes/visual/Resources.h>
#include <scapes/foundation/math/Math.h>

#include <algorithm>

using namespace scapes::foundation;

namespace scapes::visual::utils
{
	/*
	 */
	struct CubemapFaceData
	{
		math::mat4 projection;
		math::mat4 face_views[6];
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
		uint32_t ubo_size = sizeof(CubemapFaceData);
		uniform_buffer = device->createUniformBuffer(render::BufferType::DYNAMIC, ubo_size);

		// Create bind set
		bind_set = device->createBindSet();

		// Create framebuffer
		render::FrameBufferAttachment frame_buffer_attachments[1] =
		{
			{ target_texture->gpu_data, target_mip, 0, 6 },
		};

		frame_buffer = device->createFrameBuffer(1, frame_buffer_attachments);

		// Create render pass
		render::RenderPassClearValue clear_value;
		clear_value.as_color = { 0.0f, 0.0f, 0.0f, 1.0f };

		render::Multisample samples = render::Multisample::COUNT_1;
		render::RenderPassLoadOp load_op = render::RenderPassLoadOp::CLEAR;
		render::RenderPassStoreOp store_op = render::RenderPassStoreOp::STORE;

		render::RenderPassAttachment render_pass_attachments[1] =
		{
			{ target_texture->format, samples, load_op, store_op, clear_value },
		};

		uint32_t color_attachments[1] = { 0 };

		render::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 1;
		render_pass_description.color_attachments = color_attachments;

		render_pass = device->createRenderPass(1, render_pass_attachments, render_pass_description);

		// Create command buffer
		command_buffer = device->createCommandBuffer(render::CommandBufferType::PRIMARY);

		// Create pipeline state
		uint32_t target_width = std::max<uint32_t>(1, target_texture->width >> target_mip);
		uint32_t target_height = std::max<uint32_t>(1, target_texture->height >> target_mip);

		graphics_pipeline = device->createGraphicsPipeline();
		device->setViewport(graphics_pipeline, 0, 0, target_width, target_height);
		device->setScissor(graphics_pipeline, 0, 0, target_width, target_height);

		// Fill uniform buffer
		CubemapFaceData *ubo = reinterpret_cast<CubemapFaceData *>(device->map(uniform_buffer));

		const math::vec3 face_dirs[6] =
		{
			math::vec3( 1.0f,  0.0f,  0.0f),
			math::vec3(-1.0f,  0.0f,  0.0f),
			math::vec3( 0.0f,  1.0f,  0.0f),
			math::vec3( 0.0f, -1.0f,  0.0f),
			math::vec3( 0.0f,  0.0f,  1.0f),
			math::vec3( 0.0f,  0.0f, -1.0f),
		};

		const math::vec3 face_ups[6] =
		{
			math::vec3( 0.0f, -1.0f,  0.0f),
			math::vec3( 0.0f, -1.0f,  0.0f),
			math::vec3( 0.0f,  0.0f,  1.0f),
			math::vec3( 0.0f,  0.0f, -1.0f),
			math::vec3( 0.0f, -1.0f,  0.0f),
			math::vec3( 0.0f, -1.0f,  0.0f),
		};

		ubo->projection = math::perspective(math::radians(90.0f), 1.0f, 0.1f, 10.0f);

		for (int i = 0; i < 6; i++)
			ubo->face_views[i] = math::lookAt(math::vec3(0.0f), face_dirs[i], face_ups[i]);

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

		device->destroyGraphicsPipeline(graphics_pipeline);
		graphics_pipeline = nullptr;
	}

	/*
	 */
	void CubemapRenderer::render(
		const resources::Mesh *mesh,
		const resources::Shader *vertex_shader,
		const resources::Shader *geometry_shader,
		const resources::Shader *fragment_shader,
		const resources::Texture *input_texture,
		uint8_t push_constants_size,
		const uint8_t *push_constants_data
	)
	{
		device->bindTexture(bind_set, 1, input_texture->gpu_data);

		device->setPushConstants(graphics_pipeline, push_constants_size, push_constants_data);
		device->setBindSet(graphics_pipeline, 0, bind_set);
		device->setShader(graphics_pipeline, render::ShaderType::VERTEX, vertex_shader->shader);
		device->setShader(graphics_pipeline, render::ShaderType::GEOMETRY, geometry_shader->shader);
		device->setShader(graphics_pipeline, render::ShaderType::FRAGMENT, fragment_shader->shader);

		device->setVertexStream(graphics_pipeline, 0, mesh->vertex_buffer);

		device->resetCommandBuffer(command_buffer);
		device->beginCommandBuffer(command_buffer);
		device->beginRenderPass(command_buffer, render_pass, frame_buffer);

		device->setCullMode(graphics_pipeline,render::CullMode::NONE);

		device->drawIndexedPrimitiveInstanced(command_buffer, graphics_pipeline, mesh->index_buffer, mesh->num_indices);

		device->endRenderPass(command_buffer);
		device->endCommandBuffer(command_buffer);

		device->submit(command_buffer);
		device->wait(1, &command_buffer);
	}
}
