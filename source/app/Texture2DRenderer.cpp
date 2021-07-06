#include "Texture2DRenderer.h"

#include "Shader.h"
#include "Texture.h"

/*
 */
Texture2DRenderer::Texture2DRenderer(render::backend::Driver *driver)
	: driver(driver)
	, quad(driver)
{
}

/*
 */
void Texture2DRenderer::init(const Texture *target_texture)
{
	assert(target_texture != nullptr);

	// Create framebuffer
	render::backend::FrameBufferAttachment frame_buffer_attachments[1] = { target_texture->getBackend() };
	frame_buffer = driver->createFrameBuffer(1, frame_buffer_attachments);

	// Create render pass
	render::backend::RenderPassAttachment render_pass_attachments[1] =
	{
		{ target_texture->getFormat(), render::backend::Multisample::COUNT_1, render::backend::RenderPassLoadOp::DONT_CARE, render::backend::RenderPassStoreOp::STORE },
	};

	uint32_t color_attachments[1] = { 0 };

	render::backend::RenderPassDescription render_pass_description = {};
	render_pass_description.num_color_attachments = 1;
	render_pass_description.color_attachments = color_attachments;

	render_pass = driver->createRenderPass(1, render_pass_attachments, render_pass_description);

	quad.createQuad(2.0f);
	command_buffer = driver->createCommandBuffer(render::backend::CommandBufferType::PRIMARY);

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
	const Shader *vertex_shader,
	const Shader *fragment_shader
)
{
	assert(vertex_shader != nullptr);
	assert(fragment_shader != nullptr);

	driver->resetCommandBuffer(command_buffer);
	driver->beginCommandBuffer(command_buffer);

	driver->beginRenderPass(command_buffer, render_pass, frame_buffer);

	driver->clearBindSets(pipeline_state);
	driver->clearShaders(pipeline_state);
	driver->setShader(pipeline_state, render::backend::ShaderType::VERTEX, vertex_shader->getBackend());
	driver->setShader(pipeline_state, render::backend::ShaderType::FRAGMENT, fragment_shader->getBackend());

	driver->clearVertexStreams(pipeline_state);
	driver->setVertexStream(pipeline_state, 0, quad.getVertexBuffer());

	driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, quad.getIndexBuffer(), quad.getNumIndices());

	driver->endRenderPass(command_buffer);

	driver->endCommandBuffer(command_buffer);
	driver->submit(command_buffer);
	driver->wait(1, &command_buffer);
}
