#include "RenderGraph.h"

#include "Scene.h"
#include "RenderScene.h"

#include "VulkanMesh.h"
#include "VulkanSwapChain.h"
#include "VulkanShader.h"

#include <GLM/glm.hpp>

#include <cassert>

using namespace render::backend;

/*
 */
RenderGraph::~RenderGraph()
{
	shutdown();
}

/*
 */
void RenderGraph::init(const RenderScene *scene, uint32_t width, uint32_t height)
{
	initGBuffer(width, height);
	initLBuffer(width, height);

	gbuffer_pass_vertex = scene->getGBufferVertexShader();
	gbuffer_pass_fragment = scene->getGBufferFragmentShader();
}

void RenderGraph::shutdown()
{
	shutdownGBuffer();
	shutdownLBuffer();

	gbuffer_pass_vertex = nullptr;
	gbuffer_pass_fragment = nullptr;
}

void RenderGraph::initGBuffer(uint32_t width, uint32_t height)
{
	gbuffer.base_color = driver->createTexture2D(width, height, 1, Format::R8G8B8A8_UNORM);
	gbuffer.depth = driver->createTexture2D(width, height, 1, Format::R32_SFLOAT);
	gbuffer.normal = driver->createTexture2D(width, height, 1, Format::R16G16_SFLOAT);
	gbuffer.shading = driver->createTexture2D(width, height, 1, Format::R8G8_UNORM);

	FrameBufferAttachment gbuffer_attachments[4] = {
		{ FrameBufferAttachmentType::COLOR, gbuffer.base_color },
		{ FrameBufferAttachmentType::COLOR, gbuffer.depth },
		{ FrameBufferAttachmentType::COLOR, gbuffer.normal },
		{ FrameBufferAttachmentType::COLOR, gbuffer.shading },
	};

	gbuffer.framebuffer = driver->createFrameBuffer(4, gbuffer_attachments);
}

void RenderGraph::shutdownGBuffer()
{
	driver->destroyTexture(gbuffer.base_color);
	driver->destroyTexture(gbuffer.depth);
	driver->destroyTexture(gbuffer.normal);
	driver->destroyTexture(gbuffer.shading);
	driver->destroyFrameBuffer(gbuffer.framebuffer);

	memset(&gbuffer, 0, sizeof(GBuffer));
}

void RenderGraph::initLBuffer(uint32_t width, uint32_t height)
{
	lbuffer.diffuse = driver->createTexture2D(width, height, 1, Format::R16G16B16A16_SFLOAT);
	lbuffer.specular = driver->createTexture2D(width, height, 1, Format::R16G16B16A16_SFLOAT);

	FrameBufferAttachment lbuffer_attachments[2] = {
		{ FrameBufferAttachmentType::COLOR, lbuffer.diffuse },
		{ FrameBufferAttachmentType::COLOR, lbuffer.specular },
	};

	lbuffer.framebuffer = driver->createFrameBuffer(2, lbuffer_attachments);
}

void RenderGraph::shutdownLBuffer()
{

	driver->destroyTexture(lbuffer.diffuse);
	driver->destroyTexture(lbuffer.specular);
	driver->destroyFrameBuffer(lbuffer.framebuffer);

	memset(&lbuffer, 0, sizeof(LBuffer));
}

void RenderGraph::resize(uint32_t width, uint32_t height)
{
	shutdownGBuffer();
	shutdownLBuffer();

	initGBuffer(width, height);
	initLBuffer(width, height);
}

/*
 */
void RenderGraph::render(const Scene *scene, const VulkanRenderFrame &frame)
{
	renderGBuffer(scene, frame);
}

void RenderGraph::renderGBuffer(const Scene *scene, const VulkanRenderFrame &frame)
{
	assert(gbuffer_pass_vertex);
	assert(gbuffer_pass_fragment);

	RenderPassClearValue clear_values[4];
	memset(clear_values, 0, sizeof(RenderPassClearValue) * 4);

	RenderPassLoadOp load_ops[4] = { RenderPassLoadOp::DONT_CARE, RenderPassLoadOp::DONT_CARE, RenderPassLoadOp::DONT_CARE, RenderPassLoadOp::DONT_CARE };
	RenderPassStoreOp store_ops[4] = { RenderPassStoreOp::STORE, RenderPassStoreOp::STORE, RenderPassStoreOp::STORE, RenderPassStoreOp::STORE };

	RenderPassInfo info;
	info.clear_values = clear_values;
	info.load_ops = load_ops;
	info.store_ops = store_ops;

	driver->beginRenderPass(frame.command_buffer, gbuffer.framebuffer, &info);

	driver->clearPushConstants();
	driver->allocateBindSets(2);
	driver->setBindSet(0, frame.bind_set);

	driver->clearShaders();
	driver->setShader(render::backend::ShaderType::VERTEX, gbuffer_pass_vertex->getBackend());
	driver->setShader(render::backend::ShaderType::FRAGMENT, gbuffer_pass_fragment->getBackend());

	for (size_t i = 0; i < scene->getNumNodes(); ++i)
	{
		const VulkanMesh *node_mesh = scene->getNodeMesh(i);
		const glm::mat4 &node_transform = scene->getNodeWorldTransform(i);
		render::backend::BindSet *node_bindings = scene->getNodeBindings(i);

		driver->setBindSet(1, node_bindings);
		driver->setPushConstants(static_cast<uint8_t>(sizeof(glm::mat4)), &node_transform);

		driver->drawIndexedPrimitive(frame.command_buffer, node_mesh->getRenderPrimitive());
	}

	driver->endRenderPass(frame.command_buffer);
}
