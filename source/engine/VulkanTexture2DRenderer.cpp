#include "VulkanTexture2DRenderer.h"

#include "VulkanTexture.h"
#include "VulkanShader.h"


using namespace render::backend;

/*
 */
VulkanTexture2DRenderer::VulkanTexture2DRenderer(Driver *driver)
	: driver(driver)
	, quad(driver)
{
}

/*
 */
void VulkanTexture2DRenderer::init(const VulkanTexture *target_texture)
{
	assert(target_texture != nullptr);

	quad.createQuad(2.0f);

	// Create framebuffer
	FrameBufferAttachment attachment = { FrameBufferAttachmentType::COLOR, target_texture->getBackend(), 0, 1, 0, 1};
	framebuffer = driver->createFrameBuffer(1, &attachment);

	// Create command buffer
	command_buffer = driver->createCommandBuffer(CommandBufferType::PRIMARY);
}

void VulkanTexture2DRenderer::shutdown()
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
void VulkanTexture2DRenderer::render(const VulkanShader *vertex_shader, const VulkanShader *fragment_shader)
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
	driver->setShader(ShaderType::VERTEX, vertex_shader->getBackend());
	driver->setShader(ShaderType::FRAGMENT, fragment_shader->getBackend());

	driver->drawIndexedPrimitive(command_buffer, quad.getRenderPrimitive());

	driver->endRenderPass(command_buffer);

	driver->endCommandBuffer(command_buffer);
	driver->submit(command_buffer);
	driver->wait(1, &command_buffer);
}
