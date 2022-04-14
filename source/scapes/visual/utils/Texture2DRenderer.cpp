#include "Texture2DRenderer.h"

#include <scapes/visual/Resources.h>

using namespace scapes::foundation;

namespace scapes::visual::utils
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

		graphics_pipeline = device->createGraphicsPipeline();
		device->setViewport(graphics_pipeline, 0, 0, target_texture->width, target_texture->height);
		device->setScissor(graphics_pipeline, 0, 0, target_texture->width, target_texture->height);
	}

	void Texture2DRenderer::shutdown()
	{
		device->destroyFrameBuffer(frame_buffer);
		frame_buffer = nullptr;

		device->destroyRenderPass(render_pass);
		render_pass = nullptr;

		device->destroyCommandBuffer(command_buffer);
		command_buffer = nullptr;

		device->destroyGraphicsPipeline(graphics_pipeline);
		graphics_pipeline = nullptr;
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

		device->clearBindSets(graphics_pipeline);
		device->clearShaders(graphics_pipeline);
		device->setShader(graphics_pipeline, render::ShaderType::VERTEX, vertex_shader->shader);
		device->setShader(graphics_pipeline, render::ShaderType::FRAGMENT, fragment_shader->shader);

		device->clearVertexStreams(graphics_pipeline);
		device->setVertexStream(graphics_pipeline, 0, mesh->vertex_buffer);

		device->setCullMode(graphics_pipeline,render::CullMode::NONE);

		device->drawIndexedPrimitiveInstanced(command_buffer, graphics_pipeline, mesh->index_buffer, mesh->num_indices);

		device->endRenderPass(command_buffer);

		device->endCommandBuffer(command_buffer);
		device->submit(command_buffer);
		device->wait(1, &command_buffer);
	}
}
