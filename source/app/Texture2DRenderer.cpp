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
	quad.createQuad(2.0f);
	command_buffer = driver->createCommandBuffer(CommandBufferType::PRIMARY);
}

/*
 */
void Texture2DRenderer::init(const render::backend::Texture *target_texture)
{
	assert(target_texture != nullptr);

	// Create framebuffer
	FrameBufferAttachment attachment = { target_texture };
	framebuffer = driver->createFrameBuffer(1, &attachment);
}

void Texture2DRenderer::shutdown()
{
	driver->destroyFrameBuffer(framebuffer);
	framebuffer = nullptr;

	driver->destroyCommandBuffer(command_buffer);
	command_buffer = nullptr;

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

	RenderPassClearValue clear_value;
	clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};

	RenderPassLoadOp load_op = RenderPassLoadOp::CLEAR;
	RenderPassStoreOp store_op = RenderPassStoreOp::STORE;

	RenderPassInfo info;
	info.load_ops = &load_op;
	info.store_ops = &store_op;
	info.clear_values = &clear_value;

	driver->beginRenderPass(command_buffer, framebuffer, &info);

	driver->clearShaders();
	driver->setShader(ShaderType::VERTEX, vertex_shader);
	driver->setShader(ShaderType::FRAGMENT, fragment_shader);

	driver->drawIndexedPrimitive(command_buffer, quad.getRenderPrimitive());

	driver->endRenderPass(command_buffer);

	driver->endCommandBuffer(command_buffer);
	driver->submit(command_buffer);
	driver->wait(1, &command_buffer);
}
