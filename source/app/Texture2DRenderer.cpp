#include "Texture2DRenderer.h"

#include <render/Shader.h>
#include <render/Texture.h>

using namespace render;
using namespace render::backend;

/*
 */
Texture2DRenderer::Texture2DRenderer(Driver *driver)
	: driver(driver)
	, quad(driver)
{
}

/*
 */
void Texture2DRenderer::init(const render::Texture *target_texture)
{
	assert(target_texture != nullptr);

	// Create framebuffer
	FrameBufferAttachment frame_buffer_attachments[1] = { target_texture->getBackend() };
	frame_buffer = driver->createFrameBuffer(1, frame_buffer_attachments);

	// Create render pass
	RenderPassAttachment render_pass_attachments[1] =
	{
		{ target_texture->getFormat(), Multisample::COUNT_1, RenderPassLoadOp::DONT_CARE, RenderPassStoreOp::STORE },
	};

	uint32_t color_attachments[1] = { 0 };

	RenderPassDescription render_pass_description = {};
	render_pass_description.num_color_attachments = 1;
	render_pass_description.color_attachments = color_attachments;

	render_pass = driver->createRenderPass(1, render_pass_attachments, render_pass_description);

	quad.createQuad(2.0f);
	command_buffer = driver->createCommandBuffer(CommandBufferType::PRIMARY);

	pipeline_state = driver->createPipelineState();
	driver->setViewport(pipeline_state, 0, 0, target_texture->getWidth(), target_texture->getHeight());
	driver->setScissor(pipeline_state, 0, 0, target_texture->getWidth(), target_texture->getHeight());
}

void Texture2DRenderer::shutdown()
{
	driver->destroyFrameBuffer(frame_buffer);
	frame_buffer = nullptr;

	driver->destroyRenderPass(render_pass);
	render_pass = nullptr;

	driver->destroyCommandBuffer(command_buffer);
	command_buffer = nullptr;

	driver->destroyPipelineState(pipeline_state);
	pipeline_state = nullptr;

	quad.clearGPUData();
	quad.clearCPUData();
}

/*
 */
void Texture2DRenderer::render(
	const render::backend::Shader *vertex_shader,
	const render::backend::Shader *fragment_shader
)
{
	assert(vertex_shader != nullptr);
	assert(fragment_shader != nullptr);

	driver->resetCommandBuffer(command_buffer);
	driver->beginCommandBuffer(command_buffer);

	driver->beginRenderPass(command_buffer, render_pass, frame_buffer);

	driver->clearBindSets(pipeline_state);
	driver->clearShaders(pipeline_state);
	driver->setShader(pipeline_state, ShaderType::VERTEX, vertex_shader);
	driver->setShader(pipeline_state, ShaderType::FRAGMENT, fragment_shader);

	driver->clearVertexStreams(pipeline_state);
	driver->setVertexStream(pipeline_state, 0, quad.getVertexBuffer());

	driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, quad.getIndexBuffer(), quad.getNumIndices());

	driver->endRenderPass(command_buffer);

	driver->endCommandBuffer(command_buffer);
	driver->submit(command_buffer);
	driver->wait(1, &command_buffer);
}
