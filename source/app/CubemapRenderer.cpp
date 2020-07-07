#include "CubemapRenderer.h"

#include <render/Shader.h>
#include <render/Texture.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

using namespace render;
using namespace render::backend;

/*
 */
struct CubemapFaceOrientationData
{
	glm::mat4 faces[6];
};

/*
 */
CubemapRenderer::CubemapRenderer(Driver *driver)
	: driver(driver)
	, quad(driver)
{
}

/*
 */
void CubemapRenderer::init(
	const render::Texture *target_texture,
	uint32_t target_mip
)
{
	quad.createQuad(2.0f);

	// Create uniform buffers
	uint32_t ubo_size = sizeof(CubemapFaceOrientationData);
	uniform_buffer = driver->createUniformBuffer(BufferType::DYNAMIC, ubo_size);

	// Create bind set
	bind_set = driver->createBindSet();

	// Create framebuffer
	FrameBufferAttachment attachments[6] =
	{
		{ FrameBufferAttachmentType::COLOR, target_texture->getBackend(), target_mip, 1, 0, 1 },
		{ FrameBufferAttachmentType::COLOR, target_texture->getBackend(), target_mip, 1, 1, 1 },
		{ FrameBufferAttachmentType::COLOR, target_texture->getBackend(), target_mip, 1, 2, 1 },
		{ FrameBufferAttachmentType::COLOR, target_texture->getBackend(), target_mip, 1, 3, 1 },
		{ FrameBufferAttachmentType::COLOR, target_texture->getBackend(), target_mip, 1, 4, 1 },
		{ FrameBufferAttachmentType::COLOR, target_texture->getBackend(), target_mip, 1, 5, 1 },
	};

	framebuffer = driver->createFrameBuffer(6, attachments);

	// Create command buffer
	command_buffer = driver->createCommandBuffer(CommandBufferType::PRIMARY);

	// Fill uniform buffer
	CubemapFaceOrientationData *ubo = reinterpret_cast<CubemapFaceOrientationData *>(driver->map(uniform_buffer));

	const glm::mat4 &translateZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	const glm::vec3 faceDirs[6] = {
		glm::vec3( 1.0f,  0.0f,  0.0f),
		glm::vec3(-1.0f,  0.0f,  0.0f),
		glm::vec3( 0.0f,  1.0f,  0.0f),
		glm::vec3( 0.0f, -1.0f,  0.0f),
		glm::vec3( 0.0f,  0.0f,  1.0f),
		glm::vec3( 0.0f,  0.0f, -1.0f),
	};

	const glm::vec3 faceUps[6] = {
		glm::vec3( 0.0f,  0.0f, -1.0f),
		glm::vec3( 0.0f,  0.0f,  1.0f),
		glm::vec3(-1.0f,  0.0f,  0.0f),
		glm::vec3(-1.0f,  0.0f,  0.0f),
		glm::vec3( 0.0f, -1.0f,  0.0f),
		glm::vec3( 0.0f, -1.0f,  0.0f),
	};

	const glm::mat4 faceRotations[6] = {
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::mat4(1.0f),
		glm::mat4(1.0f),
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	};

	for (int i = 0; i < 6; i++)
		ubo->faces[i] = faceRotations[i] * glm::lookAtRH(glm::vec3(0.0f), faceDirs[i], faceUps[i]) * translateZ;

	driver->unmap(uniform_buffer);

	// Bind data to descriptor set
	driver->bindUniformBuffer(bind_set, 0, uniform_buffer);
}

void CubemapRenderer::shutdown()
{
	driver->destroyUniformBuffer(uniform_buffer);
	uniform_buffer = nullptr;

	driver->destroyFrameBuffer(framebuffer);
	framebuffer = nullptr;

	driver->destroyCommandBuffer(command_buffer);
	command_buffer = nullptr;

	driver->destroyBindSet(bind_set);
	bind_set = nullptr;

	quad.clearGPUData();
	quad.clearCPUData();
}

/*
 */
void CubemapRenderer::render(
	const render::Shader *vertex_shader,
	const render::Shader *fragment_shader,
	const render::Texture *input_texture,
	int input_mip,
	uint8_t push_constants_size,
	const uint8_t *push_constants_data
)
{
	if (input_mip == -1)
		driver->bindTexture(bind_set, 1, input_texture->getBackend());
	else
		driver->bindTexture(bind_set, 1, input_texture->getBackend(), static_cast<uint32_t>(input_mip), 1, 0, input_texture->getNumLayers());

	RenderPassClearValue clear_values[6];
	RenderPassLoadOp load_ops[6];
	RenderPassStoreOp store_ops[6];
	for (int i = 0; i < 6; ++i)
	{
		clear_values[i].color = {0.0f, 0.0f, 0.0f, 1.0f};
		load_ops[i] = RenderPassLoadOp::CLEAR;
		store_ops[i] = RenderPassStoreOp::STORE;
	}

	RenderPassInfo info;
	info.load_ops = load_ops;
	info.store_ops = store_ops;
	info.clear_values = clear_values;

	driver->clearPushConstants();
	if (push_constants_size > 0)
		driver->setPushConstants(push_constants_size, push_constants_data);

	driver->clearBindSets();
	driver->pushBindSet(bind_set);

	driver->resetCommandBuffer(command_buffer);
	driver->beginCommandBuffer(command_buffer);
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
