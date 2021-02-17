#include "RenderGraph.h"

#include "Scene.h"
#include "SkyLight.h"
#include "ApplicationResources.h"

#include <Tracy.hpp>

#include "ImGuiRenderer.h"
#include "imgui.h"

#include <render/Mesh.h>
#include <render/SwapChain.h>
#include <render/Shader.h>
#include <render/Texture.h>

#include <GLM/glm.hpp>

#include <cassert>

using namespace render::backend;

static float randf()
{
	return static_cast<float>(static_cast<double>(rand()) / RAND_MAX);
};



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
	imgui_renderer = new ImGuiRenderer(driver, compiler);
	imgui_renderer->init(ImGui::GetCurrentContext());

	initGBuffer(width, height);
	initLBuffer(width, height);
	initComposite(composite, width, height);
	initComposite(old_composite, width, height);

	initSSAOKernel();
	initSSAO(ssao_noised, width, height);
	initSSAO(ssao_blurred, width, height);

	initSSRData(resources->getBlueNoiseTexture());
	initSSR(ssr_trace, Format::R16G16B16A16_SFLOAT, width, height);
	initSSR(ssr_resolve, Format::R16G16B16A16_SFLOAT, width, height);
	initSSR(ssr_temporal_filter, Format::R16G16B16A16_SFLOAT, width, height);
	initSSR(old_ssr_temporal_filter, Format::R16G16B16A16_SFLOAT, width, height);

	quad = new render::Mesh(driver);
	quad->createQuad(2.0f);

	gbuffer_pass_vertex = resources->getGBufferVertexShader();
	gbuffer_pass_fragment = resources->getGBufferFragmentShader();

	ssao_pass_vertex = resources->getSSAOVertexShader();
	ssao_pass_fragment = resources->getSSAOFragmentShader();

	ssao_blur_pass_vertex = resources->getSSAOBlurVertexShader();
	ssao_blur_pass_fragment = resources->getSSAOBlurFragmentShader();

	ssr_trace_pass_vertex = resources->getSSRTraceVertexShader();
	ssr_trace_pass_fragment = resources->getSSRTraceFragmentShader();

	ssr_resolve_pass_vertex = resources->getSSRResolveVertexShader();
	ssr_resolve_pass_fragment = resources->getSSRResolveFragmentShader();

	ssr_temporal_filter_pass_vertex = resources->getSSRTemporalFilterVertexShader();
	ssr_temporal_filter_pass_fragment = resources->getSSRTemporalFilterFragmentShader();

	composite_pass_vertex = resources->getCompositeVertexShader();
	composite_pass_fragment = resources->getCompositeFragmentShader();

	final_pass_vertex = resources->getFinalVertexShader();
	final_pass_fragment = resources->getFinalFragmentShader();
}

void RenderGraph::shutdown()
{
	shutdownGBuffer();
	shutdownSSAOKernel();
	shutdownSSAO(ssao_noised);
	shutdownSSAO(ssao_blurred);
	shutdownSSRData();
	shutdownSSR(ssr_trace);
	shutdownSSR(ssr_resolve);
	shutdownSSR(ssr_temporal_filter);
	shutdownSSR(old_ssr_temporal_filter);
	shutdownLBuffer();
	shutdownComposite(composite);
	shutdownComposite(old_composite);

	delete imgui_renderer;
	imgui_renderer = nullptr;

	delete quad;
	quad = nullptr;

	gbuffer_pass_vertex = nullptr;
	gbuffer_pass_fragment = nullptr;

	ssao_pass_vertex = nullptr;
	ssao_pass_fragment = nullptr;

	ssao_blur_pass_vertex = nullptr;
	ssao_blur_pass_fragment = nullptr;

	ssr_trace_pass_vertex = nullptr;
	ssr_trace_pass_fragment = nullptr;

	ssr_resolve_pass_vertex = nullptr;
	ssr_resolve_pass_fragment = nullptr;

	ssr_temporal_filter_pass_vertex = nullptr;
	ssr_temporal_filter_pass_fragment = nullptr;

	composite_pass_vertex = nullptr;
	composite_pass_fragment = nullptr;

	final_pass_vertex = nullptr;
	final_pass_fragment = nullptr;
}

void RenderGraph::initGBuffer(uint32_t width, uint32_t height)
{
	gbuffer.base_color = driver->createTexture2D(width, height, 1, Format::R8G8B8A8_UNORM);
	gbuffer.normal = driver->createTexture2D(width, height, 1, Format::R16G16B16A16_SFLOAT);
	gbuffer.shading = driver->createTexture2D(width, height, 1, Format::R8G8_UNORM);
	gbuffer.depth = driver->createTexture2D(width, height, 1, Format::D32_SFLOAT);

	FrameBufferAttachment gbuffer_color_attachments[] = {
		{ gbuffer.base_color },
		{ gbuffer.normal },
		{ gbuffer.shading },
	};

	FrameBufferAttachment gbuffer_depth_attachments[] = {
		{ gbuffer.depth },
	};

	gbuffer.framebuffer = driver->createFrameBuffer(3, gbuffer_color_attachments, gbuffer_depth_attachments);
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

void RenderGraph::initSSAOKernel()
{
	ssao_kernel.gpu_data = driver->createUniformBuffer(BufferType::DYNAMIC, sizeof(SSAOKernel::CPUData));
	ssao_kernel.cpu_data = reinterpret_cast<SSAOKernel::CPUData *>(driver->map(ssao_kernel.gpu_data));

	uint32_t data[SSAOKernel::MAX_NOISE_SAMPLES];
	for (int i = 0; i < SSAOKernel::MAX_NOISE_SAMPLES; ++i)
	{
		const glm::vec3 &noise = glm::normalize(glm::vec3(randf(), randf(), 0.0f));
		data[i] = glm::packHalf2x16(noise);
	}

	ssao_kernel.noise_texture = driver->createTexture2D(4, 4, 1, Format::R16G16_SFLOAT, data);

	ssao_kernel.cpu_data->num_samples = 32;
	ssao_kernel.cpu_data->intensity = 1.5f;
	ssao_kernel.cpu_data->radius = 100.0f;

	buildSSAOKernel();

	ssao_kernel.bindings = driver->createBindSet();

	driver->bindUniformBuffer(ssao_kernel.bindings, 0, ssao_kernel.gpu_data);
	driver->bindTexture(ssao_kernel.bindings, 1, ssao_kernel.noise_texture);
}

void RenderGraph::buildSSAOKernel()
{
	uint32_t num_samples = ssao_kernel.cpu_data->num_samples;
	float inum_samples = 1.0f / static_cast<float>(num_samples);

	for (uint32_t i = 0; i < num_samples; ++i)
	{
		glm::vec4 sample;
		float radius = randf();
		float phi = glm::radians(randf() * 360.0f);
		float theta = glm::radians(randf() * 90.0f);

		float scale = i * inum_samples;
		scale = glm::mix(0.1f, 1.0f, scale * scale);

		radius *= scale;

		sample.x = cos(phi) * cos(theta) * radius;
		sample.y = sin(phi) * cos(theta) * radius;
		sample.z = sin(theta) * radius;
		sample.w = 1.0f;

		ssao_kernel.cpu_data->samples[i] = sample;
	}
}

void RenderGraph::shutdownSSAOKernel()
{
	driver->unmap(ssao_kernel.gpu_data);
	driver->destroyUniformBuffer(ssao_kernel.gpu_data);
	driver->destroyTexture(ssao_kernel.noise_texture);
	driver->destroyBindSet(ssao_kernel.bindings);

	memset(&ssao_kernel, 0, sizeof(SSAOKernel));
}

void RenderGraph::initSSAO(SSAO &ssao, uint32_t width, uint32_t height)
{
	ssao.texture = driver->createTexture2D(width, height, 1, Format::R8_UNORM);

	FrameBufferAttachment ssao_attachments[] = {
		{ ssao.texture },
	};

	ssao.framebuffer = driver->createFrameBuffer(1, ssao_attachments);
	ssao.bindings = driver->createBindSet();

	driver->bindTexture(ssao.bindings, 0, ssao.texture);
}

void RenderGraph::shutdownSSAO(SSAO &ssao)
{
	driver->destroyTexture(ssao.texture);
	driver->destroyFrameBuffer(ssao.framebuffer);
	driver->destroyBindSet(ssao.bindings);

	memset(&ssao, 0, sizeof(SSAO));
}

void RenderGraph::initSSRData(const render::Texture *blue_noise)
{
	ssr_data.gpu_data = driver->createUniformBuffer(BufferType::DYNAMIC, sizeof(SSRData::CPUData));
	ssr_data.cpu_data = reinterpret_cast<SSRData::CPUData *>(driver->map(ssr_data.gpu_data));

	ssr_data.cpu_data->coarse_step_size = 100.0f;
	ssr_data.cpu_data->num_coarse_steps = 8;
	ssr_data.cpu_data->num_precision_steps = 8;
	ssr_data.cpu_data->facing_threshold = 0.0f;
	ssr_data.cpu_data->bypass_depth_threshold = 1.0f;

	ssr_data.bindings = driver->createBindSet();

	driver->bindUniformBuffer(ssr_data.bindings, 0, ssr_data.gpu_data);
	driver->bindTexture(ssr_data.bindings, 1, blue_noise->getBackend());
}

void RenderGraph::shutdownSSRData()
{
	driver->unmap(ssr_data.gpu_data);
	driver->destroyUniformBuffer(ssr_data.gpu_data);
	driver->destroyBindSet(ssr_data.bindings);

	memset(&ssr_data, 0, sizeof(SSRData));
}

void RenderGraph::initSSR(SSR &ssr, Format format, uint32_t width, uint32_t height)
{
	ssr.texture = driver->createTexture2D(width, height, 1, format);

	FrameBufferAttachment ssr_attachments[] = {
		{ ssr.texture },
	};

	ssr.framebuffer = driver->createFrameBuffer(1, ssr_attachments);
	ssr.bindings = driver->createBindSet();

	driver->bindTexture(ssr.bindings, 0, ssr.texture);
}

void RenderGraph::shutdownSSR(SSR &ssr)
{
	driver->destroyTexture(ssr.texture);
	driver->destroyFrameBuffer(ssr.framebuffer);
	driver->destroyBindSet(ssr.bindings);

	memset(&ssr, 0, sizeof(SSR));
}

void RenderGraph::initLBuffer(uint32_t width, uint32_t height)
{
	lbuffer.diffuse = driver->createTexture2D(width, height, 1, Format::R16G16B16A16_SFLOAT);
	lbuffer.specular = driver->createTexture2D(width, height, 1, Format::R16G16B16A16_SFLOAT);

	FrameBufferAttachment lbuffer_attachments[] = {
		{ lbuffer.diffuse },
		{ lbuffer.specular },
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

void RenderGraph::initComposite(Composite &composite, uint32_t width, uint32_t height)
{
	uint32_t num_mipmaps = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	composite.hdr_color = driver->createTexture2D(width, height, num_mipmaps, Format::R16G16B16A16_SFLOAT);

	FrameBufferAttachment composite_attachments[] = {
		{ composite.hdr_color },
	};

	composite.framebuffer = driver->createFrameBuffer(1, composite_attachments);
	composite.bindings = driver->createBindSet();

	driver->bindTexture(composite.bindings, 0, composite.hdr_color);
}

void RenderGraph::shutdownComposite(Composite &composite)
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
	shutdownComposite(composite);
	shutdownComposite(old_composite);

	shutdownSSAO(ssao_noised);
	shutdownSSAO(ssao_blurred);

	shutdownSSR(ssr_trace);
	shutdownSSR(ssr_resolve);
	shutdownSSR(ssr_temporal_filter);
	shutdownSSR(old_ssr_temporal_filter);

	initGBuffer(width, height);
	initLBuffer(width, height);
	initComposite(composite, width, height);
	initComposite(old_composite, width, height);

	initSSAO(ssao_noised, width, height);
	initSSAO(ssao_blurred, width, height);

	initSSR(ssr_trace, Format::R16G16B16A16_SFLOAT, width, height);
	initSSR(ssr_resolve, Format::R16G16B16A16_SFLOAT, width, height);
	initSSR(ssr_temporal_filter, Format::R16G16B16A16_SFLOAT, width, height);
	initSSR(old_ssr_temporal_filter, Format::R16G16B16A16_SFLOAT, width, height);
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

	{
		ZoneScopedN("GBuffer pass");

		driver->setBlending(false);
		driver->setCullMode(render::backend::CullMode::BACK);
		driver->setDepthWrite(true);
		driver->setDepthTest(true);

		renderGBuffer(scene, frame);
	}

	{
		ZoneScopedN("SSAO");

		driver->setBlending(false);
		driver->setCullMode(render::backend::CullMode::NONE);
		driver->setDepthWrite(false);
		driver->setDepthTest(false);

		renderSSAO(scene, frame);
	}

	{
		ZoneScopedN("SSAO Blur");

		driver->setBlending(false);
		driver->setCullMode(render::backend::CullMode::NONE);
		driver->setDepthWrite(false);
		driver->setDepthTest(false);

		renderSSAOBlur(scene, frame);
	}

	{
		ZoneScopedN("LBuffer pass");

		renderLBuffer(scene, frame);
	}

	{
		ZoneScopedN("SSR Trace");

		driver->setBlending(false);
		driver->setCullMode(render::backend::CullMode::NONE);
		driver->setDepthWrite(false);
		driver->setDepthTest(false);

		renderSSRTrace(scene, frame);
	}

	{
		ZoneScopedN("SSR Resolve");

		driver->setBlending(false);
		driver->setCullMode(render::backend::CullMode::NONE);
		driver->setDepthWrite(false);
		driver->setDepthTest(false);

		renderSSRResolve(scene, frame);
	}

	{
		ZoneScopedN("SSR Temporal Filter");

		driver->setBlending(false);
		driver->setCullMode(render::backend::CullMode::NONE);
		driver->setDepthWrite(false);
		driver->setDepthTest(false);

		renderSSRTemporalFilter(scene, frame);
	}

	{
		ZoneScopedN("Composite pass");

		renderComposite(scene, frame);
	}

	{
		ZoneScopedN("Final pass");

		renderFinal(scene, frame);
	}

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

void RenderGraph::renderSSAO(const Scene *scene, const render::RenderFrame &frame)
{
	RenderPassClearValue clear_value = {};

	RenderPassLoadOp load_op = RenderPassLoadOp::DONT_CARE;
	RenderPassStoreOp store_op = RenderPassStoreOp::STORE;

	RenderPassInfo info;
	info.clear_values = &clear_value;
	info.load_ops = &load_op;
	info.store_ops = &store_op;

	driver->beginRenderPass(frame.command_buffer, ssao_noised.framebuffer, &info);

	driver->clearPushConstants();
	driver->allocateBindSets(3);
	driver->setBindSet(0, frame.bind_set);
	driver->setBindSet(1, gbuffer.bindings);
	driver->setBindSet(2, ssao_kernel.bindings);

	driver->clearShaders();
	driver->setShader(render::backend::ShaderType::VERTEX, ssao_pass_vertex->getBackend());
	driver->setShader(render::backend::ShaderType::FRAGMENT, ssao_pass_fragment->getBackend());

	driver->drawIndexedPrimitive(frame.command_buffer, quad->getRenderPrimitive());
	driver->endRenderPass(frame.command_buffer);
}

void RenderGraph::renderSSAOBlur(const Scene *scene, const render::RenderFrame &frame)
{
	RenderPassClearValue clear_value = {};

	RenderPassLoadOp load_op = RenderPassLoadOp::DONT_CARE;
	RenderPassStoreOp store_op = RenderPassStoreOp::STORE;

	RenderPassInfo info;
	info.clear_values = &clear_value;
	info.load_ops = &load_op;
	info.store_ops = &store_op;

	driver->beginRenderPass(frame.command_buffer, ssao_blurred.framebuffer, &info);

	driver->clearPushConstants();
	driver->allocateBindSets(2);
	driver->setBindSet(0, frame.bind_set);
	driver->setBindSet(1, ssao_noised.bindings);

	driver->clearShaders();
	driver->setShader(render::backend::ShaderType::VERTEX, ssao_blur_pass_vertex->getBackend());
	driver->setShader(render::backend::ShaderType::FRAGMENT, ssao_blur_pass_fragment->getBackend());

	driver->drawIndexedPrimitive(frame.command_buffer, quad->getRenderPrimitive());
	driver->endRenderPass(frame.command_buffer);
}

void RenderGraph::renderSSRTrace(const Scene *scene, const render::RenderFrame &frame)
{
	RenderPassClearValue clear_value = {};

	RenderPassLoadOp load_op = RenderPassLoadOp::DONT_CARE;
	RenderPassStoreOp store_op = RenderPassStoreOp::STORE;

	RenderPassInfo info;
	info.clear_values = &clear_value;
	info.load_ops = &load_op;
	info.store_ops = &store_op;

	driver->beginRenderPass(frame.command_buffer, ssr_trace.framebuffer, &info);

	driver->clearPushConstants();
	driver->allocateBindSets(3);
	driver->setBindSet(0, frame.bind_set);
	driver->setBindSet(1, gbuffer.bindings);
	driver->setBindSet(2, ssr_data.bindings);

	driver->clearShaders();
	driver->setShader(render::backend::ShaderType::VERTEX, ssr_trace_pass_vertex->getBackend());
	driver->setShader(render::backend::ShaderType::FRAGMENT, ssr_trace_pass_fragment->getBackend());

	driver->drawIndexedPrimitive(frame.command_buffer, quad->getRenderPrimitive());
	driver->endRenderPass(frame.command_buffer);
}

void RenderGraph::renderSSRResolve(const Scene *scene, const render::RenderFrame &frame)
{
	RenderPassClearValue clear_value = {};

	RenderPassLoadOp load_op = RenderPassLoadOp::DONT_CARE;
	RenderPassStoreOp store_op = RenderPassStoreOp::STORE;

	RenderPassInfo info;
	info.clear_values = &clear_value;
	info.load_ops = &load_op;
	info.store_ops = &store_op;

	driver->beginRenderPass(frame.command_buffer, ssr_resolve.framebuffer, &info);

	driver->clearPushConstants();
	driver->allocateBindSets(5);
	driver->setBindSet(0, frame.bind_set);
	driver->setBindSet(1, gbuffer.bindings);
	driver->setBindSet(2, ssr_trace.bindings);
	driver->setBindSet(3, ssr_data.bindings);
	driver->setBindSet(4, old_composite.bindings);

	driver->clearShaders();
	driver->setShader(render::backend::ShaderType::VERTEX, ssr_resolve_pass_vertex->getBackend());
	driver->setShader(render::backend::ShaderType::FRAGMENT, ssr_resolve_pass_fragment->getBackend());

	driver->drawIndexedPrimitive(frame.command_buffer, quad->getRenderPrimitive());
	driver->endRenderPass(frame.command_buffer);
}

void RenderGraph::renderSSRTemporalFilter(const Scene *scene, const render::RenderFrame &frame)
{
	RenderPassClearValue clear_value = {};

	RenderPassLoadOp load_op = RenderPassLoadOp::DONT_CARE;
	RenderPassStoreOp store_op = RenderPassStoreOp::STORE;

	RenderPassInfo info;
	info.clear_values = &clear_value;
	info.load_ops = &load_op;
	info.store_ops = &store_op;

	driver->beginRenderPass(frame.command_buffer, ssr_temporal_filter.framebuffer, &info);

	driver->clearPushConstants();
	driver->allocateBindSets(2);
	driver->setBindSet(0, ssr_resolve.bindings);
	driver->setBindSet(1, old_ssr_temporal_filter.bindings);

	driver->clearShaders();
	driver->setShader(render::backend::ShaderType::VERTEX, ssr_temporal_filter_pass_vertex->getBackend());
	driver->setShader(render::backend::ShaderType::FRAGMENT, ssr_temporal_filter_pass_fragment->getBackend());

	driver->drawIndexedPrimitive(frame.command_buffer, quad->getRenderPrimitive());
	driver->endRenderPass(frame.command_buffer);

	std::swap(old_ssr_temporal_filter, ssr_temporal_filter);
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
	driver->allocateBindSets(3);
	driver->setBindSet(0, lbuffer.bindings);
	driver->setBindSet(1, ssao_blurred.bindings);
	driver->setBindSet(2, ssr_temporal_filter.bindings);

	driver->clearShaders();
	driver->setShader(render::backend::ShaderType::VERTEX, composite_pass_vertex->getBackend());
	driver->setShader(render::backend::ShaderType::FRAGMENT, composite_pass_fragment->getBackend());

	// TODO: GI

	driver->drawIndexedPrimitive(frame.command_buffer, quad->getRenderPrimitive());
	driver->endRenderPass(frame.command_buffer);

	driver->generateTexture2DMipmaps(composite.hdr_color);
	std::swap(composite, old_composite);
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

	driver->beginRenderPass(frame.command_buffer, frame.swap_chain, &info);

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
