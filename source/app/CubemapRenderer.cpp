#include "CubemapRenderer.h"

#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

/*
 */
struct CubemapFaceOrientationData
{
	glm::mat4 faces[6];
};

/*
 */
CubemapRenderer::CubemapRenderer(render::backend::Driver *driver)
	: driver(driver)
{
}

CubemapRenderer::~CubemapRenderer()
{
	shutdown();
}

/*
 */
void CubemapRenderer::init(
	const Texture *target_texture,
	uint32_t target_mip
)
{
	// Create uniform buffers
	uint32_t ubo_size = sizeof(CubemapFaceOrientationData);
	uniform_buffer = driver->createUniformBuffer(render::backend::BufferType::DYNAMIC, ubo_size);

	// Create bind set
	bind_set = driver->createBindSet();

	// Create framebuffer
	render::backend::FrameBufferAttachment frame_buffer_attachments[6] =
	{
		{ target_texture->gpu_data, target_mip, 0, 1 },
		{ target_texture->gpu_data, target_mip, 1, 1 },
		{ target_texture->gpu_data, target_mip, 2, 1 },
		{ target_texture->gpu_data, target_mip, 3, 1 },
		{ target_texture->gpu_data, target_mip, 4, 1 },
		{ target_texture->gpu_data, target_mip, 5, 1 },
	};

	frame_buffer = driver->createFrameBuffer(6, frame_buffer_attachments);

	// Create render pass
	render::backend::RenderPassClearValue clear_value;
	clear_value.as_color = { 0.0f, 0.0f, 0.0f, 1.0f };

	render::backend::Multisample samples = render::backend::Multisample::COUNT_1;
	render::backend::RenderPassLoadOp load_op = render::backend::RenderPassLoadOp::CLEAR;
	render::backend::RenderPassStoreOp store_op = render::backend::RenderPassStoreOp::STORE;

	render::backend::RenderPassAttachment render_pass_attachments[6] =
	{
		{ target_texture->format, samples, load_op, store_op, clear_value },
		{ target_texture->format, samples, load_op, store_op, clear_value },
		{ target_texture->format, samples, load_op, store_op, clear_value },
		{ target_texture->format, samples, load_op, store_op, clear_value },
		{ target_texture->format, samples, load_op, store_op, clear_value },
		{ target_texture->format, samples, load_op, store_op, clear_value },
	};

	uint32_t color_attachments[6] = { 0, 1, 2, 3, 4, 5 };

	render::backend::RenderPassDescription render_pass_description = {};
	render_pass_description.num_color_attachments = 6;
	render_pass_description.color_attachments = color_attachments;

	render_pass = driver->createRenderPass(6, render_pass_attachments, render_pass_description);

	// Create command buffer
	command_buffer = driver->createCommandBuffer(render::backend::CommandBufferType::PRIMARY);

	// Create pipeline state
	pipeline_state = driver->createPipelineState();
	driver->setViewport(pipeline_state, 0, 0, target_texture->getWidth(target_mip), target_texture->getHeight(target_mip));
	driver->setScissor(pipeline_state, 0, 0, target_texture->getWidth(target_mip), target_texture->getHeight(target_mip));

	// Fill uniform buffer
	CubemapFaceOrientationData *ubo = reinterpret_cast<CubemapFaceOrientationData *>(driver->map(uniform_buffer));

	const glm::mat4 &translateZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	const glm::vec3 faceDirs[6] =
	{
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

	// TODO: get rid of this mess
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

	driver->destroyFrameBuffer(frame_buffer);
	frame_buffer = nullptr;

	driver->destroyRenderPass(render_pass);
	render_pass = nullptr;

	driver->destroyCommandBuffer(command_buffer);
	command_buffer = nullptr;

	driver->destroyBindSet(bind_set);
	bind_set = nullptr;

	driver->destroyPipelineState(pipeline_state);
	pipeline_state = nullptr;
}

/*
 */
void CubemapRenderer::render(
	const Mesh *mesh,
	const Shader *vertex_shader,
	const Shader *fragment_shader,
	const Texture *input_texture,
	uint8_t push_constants_size,
	const uint8_t *push_constants_data
)
{
	driver->bindTexture(bind_set, 1, input_texture->gpu_data);

	driver->setPushConstants(pipeline_state, push_constants_size, push_constants_data);
	driver->setBindSet(pipeline_state, 0, bind_set);
	driver->setShader(pipeline_state, render::backend::ShaderType::VERTEX, vertex_shader->getBackend());
	driver->setShader(pipeline_state, render::backend::ShaderType::FRAGMENT, fragment_shader->getBackend());

	driver->setVertexStream(pipeline_state, 0, mesh->vertex_buffer);

	driver->resetCommandBuffer(command_buffer);
	driver->beginCommandBuffer(command_buffer);
	driver->beginRenderPass(command_buffer, render_pass, frame_buffer);

	driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, mesh->index_buffer, mesh->num_indices);

	driver->endRenderPass(command_buffer);
	driver->endCommandBuffer(command_buffer);

	driver->submit(command_buffer);
	driver->wait(1, &command_buffer);
}
