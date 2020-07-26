#include "RenderGraph.h"

#include "Scene.h"
#include "SkyLight.h"
#include "ApplicationResources.h"

#include "ImGuiRenderer.h"
#include "imgui.h"

#include <render/Mesh.h>
#include <render/SwapChain.h>
#include <render/Shader.h>

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
void RenderGraph::init(const ApplicationResources *resources, uint32_t width, uint32_t height)
{
	imgui_renderer = new ImGuiRenderer(driver);
	imgui_renderer->init(ImGui::GetCurrentContext());

	initGBuffer(width, height);
	initLBuffer(width, height);
	initComposite(width, height);

	quad = new render::Mesh(driver);
	quad->createQuad(2.0f);

	gbuffer_pass_vertex = resources->getGBufferVertexShader();
	gbuffer_pass_fragment = resources->getGBufferFragmentShader();

	composite_pass_vertex = resources->getCompositeVertexShader();
	composite_pass_fragment = resources->getCompositeFragmentShader();

	final_pass_vertex = resources->getFinalVertexShader();
	final_pass_fragment = resources->getFinalFragmentShader();
}

void RenderGraph::shutdown()
{
	shutdownGBuffer();
	shutdownLBuffer();
	shutdownComposite();

	delete imgui_renderer;
	imgui_renderer = nullptr;

	delete quad;
	quad = nullptr;

	gbuffer_pass_vertex = nullptr;
	gbuffer_pass_fragment = nullptr;

	composite_pass_vertex = nullptr;
	composite_pass_fragment = nullptr;

	final_pass_vertex = nullptr;
	final_pass_fragment = nullptr;
}

void RenderGraph::initGBuffer(uint32_t width, uint32_t height)
{
	gbuffer.base_color = driver->createTexture2D(width, height, 1, Format::R8G8B8A8_UNORM);
	gbuffer.normal = driver->createTexture2D(width, height, 1, Format::R16G16_SFLOAT);
	gbuffer.shading = driver->createTexture2D(width, height, 1, Format::R8G8_UNORM);
	gbuffer.depth = driver->createTexture2D(width, height, 1, Format::D32_SFLOAT);

	FrameBufferAttachment gbuffer_attachments[4] = {
		{ FrameBufferAttachmentType::COLOR, gbuffer.base_color },
		{ FrameBufferAttachmentType::COLOR, gbuffer.normal },
		{ FrameBufferAttachmentType::COLOR, gbuffer.shading },
		{ FrameBufferAttachmentType::DEPTH, gbuffer.depth },
	};

	gbuffer.framebuffer = driver->createFrameBuffer(4, gbuffer_attachments);
	gbuffer.bindings = driver->createBindSet();

	driver->bindTexture(gbuffer.bindings, 0, gbuffer.base_color);
	driver->bindTexture(gbuffer.bindings, 1, gbuffer.normal);
	driver->bindTexture(gbuffer.bindings, 2, gbuffer.shading);
	driver->bindTexture(gbuffer.bindings, 3, gbuffer.depth);
}

void RenderGraph::shutdownGBuffer()
{
	driver->destroyTexture(gbuffer.base_color);
	driver->destroyTexture(gbuffer.depth);
	driver->destroyTexture(gbuffer.normal);
	driver->destroyTexture(gbuffer.shading);
	driver->destroyFrameBuffer(gbuffer.framebuffer);
	driver->destroyBindSet(gbuffer.bindings);

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
	lbuffer.bindings = driver->createBindSet();

	driver->bindTexture(lbuffer.bindings, 0, lbuffer.diffuse);
	driver->bindTexture(lbuffer.bindings, 1, lbuffer.specular);
}

void RenderGraph::shutdownLBuffer()
{
	driver->destroyTexture(lbuffer.diffuse);
	driver->destroyTexture(lbuffer.specular);
	driver->destroyFrameBuffer(lbuffer.framebuffer);
	driver->destroyBindSet(lbuffer.bindings);

	memset(&lbuffer, 0, sizeof(LBuffer));
}

void RenderGraph::initComposite(uint32_t width, uint32_t height)
{
	composite.hdr_color = driver->createTexture2D(width, height, 1, Format::R16G16B16A16_SFLOAT);

	FrameBufferAttachment composite_attachments[1] = {
		{ FrameBufferAttachmentType::COLOR, composite.hdr_color },
	};

	composite.framebuffer = driver->createFrameBuffer(1, composite_attachments);
	composite.bindings = driver->createBindSet();

	driver->bindTexture(composite.bindings, 0, composite.hdr_color);
}

void RenderGraph::shutdownComposite()
{
	driver->destroyTexture(composite.hdr_color);
	driver->destroyFrameBuffer(composite.framebuffer);
	driver->destroyBindSet(composite.bindings);

	memset(&composite, 0, sizeof(Composite));
}

void RenderGraph::resize(uint32_t width, uint32_t height)
{
	imgui_renderer->invalidateTextureIDs();

	shutdownGBuffer();
	shutdownLBuffer();
	shutdownComposite();

	initGBuffer(width, height);
	initLBuffer(width, height);
	initComposite(width, height);
}

/*
 */
ImTextureID RenderGraph::fetchTextureID(const render::backend::Texture *texture)
{
	assert(imgui_renderer);

	return imgui_renderer->fetchTextureID(texture);
}

/*
 */
void RenderGraph::render(const Scene *scene, const render::RenderFrame &frame)
{
	driver->resetCommandBuffer(frame.command_buffer);
	driver->beginCommandBuffer(frame.command_buffer);

	driver->setBlending(false);
	driver->setCullMode(render::backend::CullMode::BACK);
	driver->setDepthWrite(true);
	driver->setDepthTest(true);

	renderGBuffer(scene, frame);

	driver->setBlending(false);
	driver->setCullMode(render::backend::CullMode::NONE);
	driver->setDepthWrite(false);
	driver->setDepthTest(false);

	renderLBuffer(scene, frame);
	renderComposite(scene, frame);
	renderFinal(scene, frame);

	driver->endCommandBuffer(frame.command_buffer);
}

void RenderGraph::renderGBuffer(const Scene *scene, const render::RenderFrame &frame)
{
	assert(gbuffer_pass_vertex);
	assert(gbuffer_pass_fragment);

	RenderPassClearValue clear_values[4];
	memset(clear_values, 0, sizeof(RenderPassClearValue) * 4);

	clear_values[3].depth_stencil = {1.0f, 0};

	RenderPassLoadOp load_ops[4] = { RenderPassLoadOp::CLEAR, RenderPassLoadOp::CLEAR, RenderPassLoadOp::CLEAR, RenderPassLoadOp::CLEAR };
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
		const render::Mesh *node_mesh = scene->getNodeMesh(i);
		const glm::mat4 &node_transform = scene->getNodeWorldTransform(i);
		render::backend::BindSet *node_bindings = scene->getNodeBindings(i);

		driver->setBindSet(1, node_bindings);
		driver->setPushConstants(static_cast<uint8_t>(sizeof(glm::mat4)), &node_transform);

		driver->drawIndexedPrimitive(frame.command_buffer, node_mesh->getRenderPrimitive());
	}

	driver->endRenderPass(frame.command_buffer);
}

void RenderGraph::renderLBuffer(const Scene *scene, const render::RenderFrame &frame)
{
	RenderPassClearValue clear_values[2];
	memset(clear_values, 0, sizeof(RenderPassClearValue) * 2);

	RenderPassLoadOp load_ops[2] = { RenderPassLoadOp::DONT_CARE, RenderPassLoadOp::DONT_CARE };
	RenderPassStoreOp store_ops[2] = { RenderPassStoreOp::STORE, RenderPassStoreOp::STORE };

	RenderPassInfo info;
	info.clear_values = clear_values;
	info.load_ops = load_ops;
	info.store_ops = store_ops;

	driver->beginRenderPass(frame.command_buffer, lbuffer.framebuffer, &info);

	driver->clearPushConstants();
	driver->allocateBindSets(3);
	driver->setBindSet(0, frame.bind_set);
	driver->setBindSet(1, gbuffer.bindings);

	for (size_t i = 0; i < scene->getNumLights(); ++i)
	{
		const Light *light = scene->getLight(i);

		const render::Shader *vertex_shader = light->getVertexShader();
		const render::Shader *fragment_shader = light->getFragmentShader();
		const render::Mesh *light_mesh = light->getMesh();
		render::backend::BindSet *light_bindings = light->getBindSet();

		driver->clearShaders();
		driver->setShader(render::backend::ShaderType::VERTEX, vertex_shader->getBackend());
		driver->setShader(render::backend::ShaderType::FRAGMENT, fragment_shader->getBackend());

		driver->setBindSet(2, light_bindings);
		driver->drawIndexedPrimitive(frame.command_buffer, light_mesh->getRenderPrimitive());
	}

	driver->endRenderPass(frame.command_buffer);
}

void RenderGraph::renderComposite(const Scene *scene, const render::RenderFrame &frame)
{
	assert(composite_pass_vertex);
	assert(composite_pass_fragment);

	RenderPassClearValue clear_values[1];
	memset(clear_values, 0, sizeof(RenderPassClearValue));

	RenderPassLoadOp load_ops[1] = { RenderPassLoadOp::DONT_CARE };
	RenderPassStoreOp store_ops[1] = { RenderPassStoreOp::STORE };

	RenderPassInfo info;
	info.clear_values = clear_values;
	info.load_ops = load_ops;
	info.store_ops = store_ops;

	driver->beginRenderPass(frame.command_buffer, composite.framebuffer, &info);

	driver->clearPushConstants();
	driver->allocateBindSets(1);
	driver->setBindSet(0, lbuffer.bindings);

	driver->clearShaders();
	driver->setShader(render::backend::ShaderType::VERTEX, composite_pass_vertex->getBackend());
	driver->setShader(render::backend::ShaderType::FRAGMENT, composite_pass_fragment->getBackend());

	// TODO: SSAO
	// TODO: IBL + GI
	// TODO: SSR

	driver->drawIndexedPrimitive(frame.command_buffer, quad->getRenderPrimitive());
	driver->endRenderPass(frame.command_buffer);
}

void RenderGraph::renderFinal(const Scene *scene, const render::RenderFrame &frame)
{
	assert(final_pass_vertex);
	assert(final_pass_fragment);

	RenderPassClearValue clear_values[3];
	clear_values[0].color = {0.2f, 0.2f, 0.2f, 1.0f};
	clear_values[1].color = {0.0f, 0.0f, 0.0f, 1.0f};
	clear_values[2].depth_stencil = {1.0f, 0};

	RenderPassLoadOp load_ops[3] = { RenderPassLoadOp::CLEAR, RenderPassLoadOp::DONT_CARE, RenderPassLoadOp::CLEAR };
	RenderPassStoreOp store_ops[3] = { RenderPassStoreOp::STORE, RenderPassStoreOp::STORE, RenderPassStoreOp::DONT_CARE };

	RenderPassInfo info;
	info.load_ops = load_ops;
	info.store_ops = store_ops;
	info.clear_values = clear_values;

	driver->beginRenderPass(frame.command_buffer, frame.frame_buffer, &info);

	driver->clearPushConstants();
	driver->clearBindSets();
	driver->pushBindSet(composite.bindings);

	driver->clearShaders();
	driver->setShader(render::backend::ShaderType::VERTEX, final_pass_vertex->getBackend());
	driver->setShader(render::backend::ShaderType::FRAGMENT, final_pass_fragment->getBackend());

	driver->drawIndexedPrimitive(frame.command_buffer, quad->getRenderPrimitive());

	imgui_renderer->render(frame);

	driver->endRenderPass(frame.command_buffer);
}
