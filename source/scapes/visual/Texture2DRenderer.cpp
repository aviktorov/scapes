#include "Texture2DRenderer.h"

#include <scapes/visual/Resources.h>

using namespace scapes::foundation;

namespace scapes::visual
{
	/*
	 */
	Texture2DRenderer::Texture2DRenderer(render::Device *device)
		: device(device)
	{
	}

	Texture2DRenderer::~Texture2DRenderer()
	{
		shutdown();
	}

	/*
	 */
	void Texture2DRenderer::init(const resources::Texture *target_texture)
	{
		assert(target_texture != nullptr);

		// Create framebuffer
		render::FrameBufferAttachment frame_buffer_attachments[1] = { target_texture->gpu_data };
		frame_buffer = device->createFrameBuffer(1, frame_buffer_attachments);

		// Create render pass
		render::RenderPassAttachment render_pass_attachments[1] =
		{
			{ target_texture->format, render::Multisample::COUNT_1, render::RenderPassLoadOp::DONT_CARE, render::RenderPassStoreOp::STORE },
		};

		uint32_t color_attachments[1] = { 0 };

		render::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 1;
		render_pass_description.color_attachments = color_attachments;

		render_pass = device->createRenderPass(1, render_pass_attachments, render_pass_description);

		command_buffer = device->createCommandBuffer(render::CommandBufferType::PRIMARY);

		pipeline_state = device->createPipelineState();
		device->setViewport(pipeline_state, 0, 0, target_texture->width, target_texture->height);
		device->setScissor(pipeline_state, 0, 0, target_texture->width, target_texture->height);
	}

	void Texture2DRenderer::shutdown()
	{
		device->destroyFrameBuffer(frame_buffer);
		frame_buffer = nullptr;

		device->destroyRenderPass(render_pass);
		render_pass = nullptr;

		device->destroyCommandBuffer(command_buffer);
		command_buffer = nullptr;

		device->destroyPipelineState(pipeline_state);
		pipeline_state = nullptr;
	}

	/*
	 */
	void Texture2DRenderer::render(
		const resources::Mesh *mesh,
		const resources::Shader *vertex_shader,
		const resources::Shader *fragment_shader
	)
	{
		assert(vertex_shader != nullptr);
		assert(fragment_shader != nullptr);

		device->resetCommandBuffer(command_buffer);
		device->beginCommandBuffer(command_buffer);

		device->beginRenderPass(command_buffer, render_pass, frame_buffer);

		device->clearBindSets(pipeline_state);
		device->clearShaders(pipeline_state);
		device->setShader(pipeline_state, render::ShaderType::VERTEX, vertex_shader->shader);
		device->setShader(pipeline_state, render::ShaderType::FRAGMENT, fragment_shader->shader);

		device->clearVertexStreams(pipeline_state);
		device->setVertexStream(pipeline_state, 0, mesh->vertex_buffer);

		device->setCullMode(pipeline_state,render::CullMode::NONE);

		device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, mesh->index_buffer, mesh->num_indices);

		device->endRenderPass(command_buffer);

		device->endCommandBuffer(command_buffer);
		device->submit(command_buffer);
		device->wait(1, &command_buffer);
	}
}
