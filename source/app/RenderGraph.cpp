#include "RenderGraph.h"
#include "RenderModule.h"
#include "ApplicationResources.h"

#include <game/World.h>
#include <Tracy.hpp>

#include "ImGuiRenderer.h"
#include "imgui.h"

#include "Mesh.h"
#include "SwapChain.h"
#include "Shader.h"
#include "Texture.h"

#include <cassert>

// TODO: move to utils
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
	pipeline_state = driver->createPipelineState();
	driver->setViewport(pipeline_state, 0, 0, width, height);
	driver->setScissor(pipeline_state, 0, 0, width, height);

	imgui_renderer = new ImGuiRenderer(driver, compiler);
	imgui_renderer->init(ImGui::GetCurrentContext());

	initRenderPasses();
	initSSAOKernel();
	initSSRData(resources->getBlueNoiseTexture().get());
	initTransient(width, height);

	quad = resources->getFullscreenQuad();

	gbuffer_pass_vertex = resources->getShader(config::Shaders::GBufferVertex);
	gbuffer_pass_fragment = resources->getShader(config::Shaders::GBufferFragment);

	fullscreen_quad_vertex = resources->getShader(config::Shaders::FullscreenQuadVertex);

	ssao_pass_fragment = resources->getShader(config::Shaders::SSAOFragment);
	ssao_blur_pass_fragment = resources->getShader(config::Shaders::SSAOBlurFragment);
	ssr_trace_pass_fragment = resources->getShader(config::Shaders::SSRTraceFragment);
	ssr_resolve_pass_fragment = resources->getShader(config::Shaders::SSRResolveFragment);
	temporal_filter_pass_fragment = resources->getShader(config::Shaders::TemporalFilterFragment);
	composite_pass_fragment = resources->getShader(config::Shaders::CompositeFragment);
	tonemapping_pass_fragment = resources->getShader(config::Shaders::TonemappingFragment);
}

void RenderGraph::shutdown()
{
	shutdownRenderPasses();
	shutdownSSAOKernel();
	shutdownSSRData();
	shutdownTransient();

	driver->destroyPipelineState(pipeline_state);
	pipeline_state = nullptr;

	delete imgui_renderer;
	imgui_renderer = nullptr;

	quad = nullptr;

	gbuffer_pass_vertex = nullptr;
	gbuffer_pass_fragment = nullptr;

	fullscreen_quad_vertex = nullptr;

	ssao_pass_fragment = nullptr;
	ssao_blur_pass_fragment = nullptr;
	ssr_trace_pass_fragment = nullptr;
	ssr_resolve_pass_fragment = nullptr;
	temporal_filter_pass_fragment = nullptr;
	composite_pass_fragment = nullptr;
	tonemapping_pass_fragment = nullptr;
}

void RenderGraph::initRenderPasses()
{
	render::backend::Multisample samples = render::backend::Multisample::COUNT_1;

	{ // GBuffer
		render::backend::RenderPassClearValue clear_depth;
		clear_depth.as_depth_stencil = { 1.0f, 0 };

		render::backend::RenderPassClearValue clear_color;
		clear_color.as_color = { 0.0f, 0.0f, 0.0f, 1.0f };

		render::backend::RenderPassAttachment render_pass_attachments[5] =
		{
			{ render::backend::Format::D32_SFLOAT, samples, render::backend::RenderPassLoadOp::CLEAR, render::backend::RenderPassStoreOp::STORE, clear_depth },
			{ render::backend::Format::R8G8B8A8_UNORM, samples, render::backend::RenderPassLoadOp::CLEAR, render::backend::RenderPassStoreOp::STORE, clear_color },
			{ render::backend::Format::R16G16B16A16_SFLOAT, samples, render::backend::RenderPassLoadOp::CLEAR, render::backend::RenderPassStoreOp::STORE, clear_color },
			{ render::backend::Format::R8G8_UNORM, samples, render::backend::RenderPassLoadOp::CLEAR, render::backend::RenderPassStoreOp::STORE, clear_color },
			{ render::backend::Format::R16G16_SFLOAT, samples, render::backend::RenderPassLoadOp::CLEAR, render::backend::RenderPassStoreOp::STORE, clear_color },
		};

		uint32_t color_attachments[4] = { 1, 2, 3, 4 };
		uint32_t depthstencil_attachment[1] = { 0 };

		render::backend::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 4;
		render_pass_description.color_attachments = color_attachments;
		render_pass_description.depthstencil_attachment = depthstencil_attachment;

		gbuffer_render_pass = driver->createRenderPass(5, render_pass_attachments, render_pass_description);
	}

	{ // LBuffer
		render::backend::RenderPassAttachment render_pass_attachments[2] =
		{
			{ render::backend::Format::R16G16B16A16_SFLOAT, samples, render::backend::RenderPassLoadOp::DONT_CARE, render::backend::RenderPassStoreOp::STORE },
			{ render::backend::Format::R16G16B16A16_SFLOAT, samples, render::backend::RenderPassLoadOp::DONT_CARE, render::backend::RenderPassStoreOp::STORE },
		};

		uint32_t color_attachments[2] = { 0, 1 };

		render::backend::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 2;
		render_pass_description.color_attachments = color_attachments;

		lbuffer_render_pass = driver->createRenderPass(2, render_pass_attachments, render_pass_description);
	}

	{ // SSR Resolve
		render::backend::RenderPassAttachment render_pass_attachments[2] =
		{
			{ render::backend::Format::R16G16B16A16_SFLOAT, samples, render::backend::RenderPassLoadOp::DONT_CARE, render::backend::RenderPassStoreOp::STORE },
			{ render::backend::Format::R16G16_SFLOAT, samples, render::backend::RenderPassLoadOp::DONT_CARE, render::backend::RenderPassStoreOp::STORE },
		};

		uint32_t color_attachments[2] = { 0, 1 };

		render::backend::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 2;
		render_pass_description.color_attachments = color_attachments;

		ssr_resolve_render_pass = driver->createRenderPass(2, render_pass_attachments, render_pass_description);
	}

	{ // SSAO
		render::backend::RenderPassAttachment render_pass_attachments[1] =
		{
			{ render::backend::Format::R8_UNORM, samples, render::backend::RenderPassLoadOp::DONT_CARE, render::backend::RenderPassStoreOp::STORE },
		};

		uint32_t color_attachments[1] = { 0 };

		render::backend::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 1;
		render_pass_description.color_attachments = color_attachments;

		ssao_render_pass = driver->createRenderPass(1, render_pass_attachments, render_pass_description);
	}

	{ // HDR
		render::backend::RenderPassAttachment render_pass_attachments[1] =
		{
			{ render::backend::Format::R16G16B16A16_SFLOAT, samples, render::backend::RenderPassLoadOp::DONT_CARE, render::backend::RenderPassStoreOp::STORE },
		};

		uint32_t color_attachments[1] = { 0 };

		render::backend::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 1;
		render_pass_description.color_attachments = color_attachments;

		hdr_render_pass = driver->createRenderPass(1, render_pass_attachments, render_pass_description);
	}

	{ // HDR Clear
		render::backend::RenderPassClearValue clear_color;
		clear_color.as_color = { 0.0f, 0.0f, 0.0f, 0.0f };

		render::backend::RenderPassAttachment render_pass_attachments[1] =
		{
			{ render::backend::Format::R16G16B16A16_SFLOAT, samples, render::backend::RenderPassLoadOp::CLEAR, render::backend::RenderPassStoreOp::STORE, clear_color },
		};

		uint32_t color_attachments[1] = { 0 };

		render::backend::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 1;
		render_pass_description.color_attachments = color_attachments;

		hdr_clear_render_pass = driver->createRenderPass(1, render_pass_attachments, render_pass_description);
	}
}

void RenderGraph::shutdownRenderPasses()
{
	driver->destroyRenderPass(gbuffer_render_pass);
	gbuffer_render_pass = nullptr;

	driver->destroyRenderPass(lbuffer_render_pass);
	lbuffer_render_pass = nullptr;

	driver->destroyRenderPass(ssao_render_pass);
	ssao_render_pass = nullptr;

	driver->destroyRenderPass(ssr_resolve_render_pass);
	ssr_resolve_render_pass = nullptr;

	driver->destroyRenderPass(hdr_render_pass);
	hdr_render_pass = nullptr;

	driver->destroyRenderPass(hdr_clear_render_pass);
	hdr_clear_render_pass = nullptr;

	driver->destroyRenderPass(swap_chain_render_pass);
	swap_chain_render_pass = nullptr;
}

void RenderGraph::initTransient(uint32_t width, uint32_t height)
{
	initGBuffer(width, height);
	initLBuffer(width, height);
	initRenderBuffer(composite_temp, render::backend::Format::R16G16B16A16_SFLOAT, width, height);
	initRenderBuffer(composite, render::backend::Format::R16G16B16A16_SFLOAT, width, height);
	initRenderBuffer(old_composite, render::backend::Format::R16G16B16A16_SFLOAT, width, height);

	initRenderBuffer(ssao_noised, render::backend::Format::R8_UNORM, width, height);
	initRenderBuffer(ssao_blurred, render::backend::Format::R8_UNORM, width, height);

	initRenderBuffer(ssr_trace, render::backend::Format::R16G16B16A16_SFLOAT, width, height);
	initSSRResolve(ssr_resolve, width, height);
	initRenderBuffer(ssr, render::backend::Format::R16G16B16A16_SFLOAT, width, height);
	initRenderBuffer(old_ssr, render::backend::Format::R16G16B16A16_SFLOAT, width, height);

	first_frame = true;
}

void RenderGraph::shutdownTransient()
{
	shutdownGBuffer();

	shutdownRenderBuffer(ssao_noised);
	shutdownRenderBuffer(ssao_blurred);

	shutdownSSRResolve(ssr_resolve);
	shutdownRenderBuffer(ssr_trace);
	shutdownRenderBuffer(ssr);
	shutdownRenderBuffer(old_ssr);

	shutdownLBuffer();
	shutdownRenderBuffer(composite_temp);
	shutdownRenderBuffer(composite);
	shutdownRenderBuffer(old_composite);
}

void RenderGraph::initGBuffer(uint32_t width, uint32_t height)
{
	gbuffer.base_color = driver->createTexture2D(width, height, 1, render::backend::Format::R8G8B8A8_UNORM);
	gbuffer.normal = driver->createTexture2D(width, height, 1, render::backend::Format::R16G16B16A16_SFLOAT);
	gbuffer.shading = driver->createTexture2D(width, height, 1, render::backend::Format::R8G8_UNORM);
	gbuffer.depth = driver->createTexture2D(width, height, 1, render::backend::Format::D32_SFLOAT);
	gbuffer.velocity = driver->createTexture2D(width, height, 1, render::backend::Format::R16G16_SFLOAT);

	render::backend::FrameBufferAttachment gbuffer_attachments[5] = {
		{ gbuffer.depth },
		{ gbuffer.base_color },
		{ gbuffer.normal },
		{ gbuffer.shading },
		{ gbuffer.velocity },
	};

	gbuffer.frame_buffer = driver->createFrameBuffer(5, gbuffer_attachments);
	gbuffer.bindings = driver->createBindSet();

	driver->bindTexture(gbuffer.bindings, 0, gbuffer.base_color);
	driver->bindTexture(gbuffer.bindings, 1, gbuffer.normal);
	driver->bindTexture(gbuffer.bindings, 2, gbuffer.shading);
	driver->bindTexture(gbuffer.bindings, 3, gbuffer.depth);

	gbuffer.velocity_bindings = driver->createBindSet();

	driver->bindTexture(gbuffer.velocity_bindings, 0, gbuffer.velocity);
}

void RenderGraph::shutdownGBuffer()
{
	driver->destroyTexture(gbuffer.base_color);
	driver->destroyTexture(gbuffer.depth);
	driver->destroyTexture(gbuffer.normal);
	driver->destroyTexture(gbuffer.shading);
	driver->destroyTexture(gbuffer.velocity);

	driver->destroyFrameBuffer(gbuffer.frame_buffer);

	driver->destroyBindSet(gbuffer.bindings);
	driver->destroyBindSet(gbuffer.velocity_bindings);

	memset(&gbuffer, 0, sizeof(GBuffer));
}

void RenderGraph::initSSAOKernel()
{
	ssao_kernel.gpu_data = driver->createUniformBuffer(render::backend::BufferType::DYNAMIC, sizeof(SSAOKernel::CPUData));
	ssao_kernel.cpu_data = reinterpret_cast<SSAOKernel::CPUData *>(driver->map(ssao_kernel.gpu_data));

	uint32_t data[SSAOKernel::MAX_NOISE_SAMPLES];
	for (int i = 0; i < SSAOKernel::MAX_NOISE_SAMPLES; ++i)
	{
		const glm::vec3 &noise = glm::normalize(glm::vec3(randf(), randf(), 0.0f));
		data[i] = glm::packHalf2x16(noise);
	}

	ssao_kernel.noise_texture = driver->createTexture2D(4, 4, 1, render::backend::Format::R16G16_SFLOAT, data);

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

void RenderGraph::initSSRData(const Texture *blue_noise)
{
	ssr_data.gpu_data = driver->createUniformBuffer(render::backend::BufferType::DYNAMIC, sizeof(SSRData::CPUData));
	ssr_data.cpu_data = reinterpret_cast<SSRData::CPUData *>(driver->map(ssr_data.gpu_data));

	ssr_data.cpu_data->coarse_step_size = 100.0f;
	ssr_data.cpu_data->num_coarse_steps = 8;
	ssr_data.cpu_data->num_precision_steps = 8;
	ssr_data.cpu_data->facing_threshold = 0.0f;
	ssr_data.cpu_data->bypass_depth_threshold = 1.0f;

	ssr_data.bindings = driver->createBindSet();

	driver->bindUniformBuffer(ssr_data.bindings, 0, ssr_data.gpu_data);
	driver->bindTexture(ssr_data.bindings, 1, blue_noise->gpu_data);
}

void RenderGraph::shutdownSSRData()
{
	driver->unmap(ssr_data.gpu_data);
	driver->destroyUniformBuffer(ssr_data.gpu_data);
	driver->destroyBindSet(ssr_data.bindings);

	memset(&ssr_data, 0, sizeof(SSRData));
}

void RenderGraph::initRenderBuffer(RenderBuffer &ssr, render::backend::Format format, uint32_t width, uint32_t height)
{
	ssr.texture = driver->createTexture2D(width, height, 1, format);

	render::backend::FrameBufferAttachment ssr_attachments[1] = {
		{ ssr.texture },
	};

	ssr.frame_buffer = driver->createFrameBuffer(1, ssr_attachments);
	ssr.bindings = driver->createBindSet();

	driver->bindTexture(ssr.bindings, 0, ssr.texture);
}

void RenderGraph::shutdownRenderBuffer(RenderBuffer &ssr)
{
	driver->destroyTexture(ssr.texture);
	driver->destroyFrameBuffer(ssr.frame_buffer);
	driver->destroyBindSet(ssr.bindings);

	memset(&ssr, 0, sizeof(RenderBuffer));
}

void RenderGraph::initSSRResolve(SSRResolve &ssr, uint32_t width, uint32_t height)
{
	ssr.resolve = driver->createTexture2D(width, height, 1, render::backend::Format::R16G16B16A16_SFLOAT);
	ssr.velocity = driver->createTexture2D(width, height, 1, render::backend::Format::R16G16_SFLOAT);

	render::backend::FrameBufferAttachment ssr_attachments[2] = {
		{ ssr.resolve },
		{ ssr.velocity },
	};

	ssr.frame_buffer = driver->createFrameBuffer(2, ssr_attachments);

	ssr.resolve_bindings = driver->createBindSet();
	driver->bindTexture(ssr.resolve_bindings, 0, ssr.resolve);

	ssr.velocity_bindings = driver->createBindSet();
	driver->bindTexture(ssr.velocity_bindings, 0, ssr.velocity);
}

void RenderGraph::shutdownSSRResolve(SSRResolve &ssr)
{
	driver->destroyFrameBuffer(ssr.frame_buffer);
	driver->destroyBindSet(ssr.resolve_bindings);
	driver->destroyBindSet(ssr.velocity_bindings);
	driver->destroyTexture(ssr.resolve);
	driver->destroyTexture(ssr.velocity);

	memset(&ssr, 0, sizeof(SSRResolve));
}

void RenderGraph::initLBuffer(uint32_t width, uint32_t height)
{
	lbuffer.diffuse = driver->createTexture2D(width, height, 1, render::backend::Format::R16G16B16A16_SFLOAT);
	lbuffer.specular = driver->createTexture2D(width, height, 1, render::backend::Format::R16G16B16A16_SFLOAT);

	render::backend::FrameBufferAttachment lbuffer_attachments[2] = {
		{ lbuffer.diffuse },
		{ lbuffer.specular },
	};

	lbuffer.frame_buffer = driver->createFrameBuffer(2, lbuffer_attachments);
	lbuffer.bindings = driver->createBindSet();

	driver->bindTexture(lbuffer.bindings, 0, lbuffer.diffuse);
	driver->bindTexture(lbuffer.bindings, 1, lbuffer.specular);
}

void RenderGraph::shutdownLBuffer()
{
	driver->destroyTexture(lbuffer.diffuse);
	driver->destroyTexture(lbuffer.specular);
	driver->destroyFrameBuffer(lbuffer.frame_buffer);
	driver->destroyBindSet(lbuffer.bindings);

	memset(&lbuffer, 0, sizeof(LBuffer));
}

void RenderGraph::resize(uint32_t width, uint32_t height)
{
	imgui_renderer->invalidateTextureIDs();

	shutdownTransient();
	initTransient(width, height);

	driver->setViewport(pipeline_state, 0, 0, width, height);
	driver->setScissor(pipeline_state, 0, 0, width, height);
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
void RenderGraph::render(render::backend::CommandBuffer *command_buffer, render::backend::SwapChain *swap_chain, const game::World *world, render::backend::BindSet *application_bindings, render::backend::BindSet *camera_bindings)
{
	driver->resetCommandBuffer(command_buffer);
	driver->beginCommandBuffer(command_buffer);

	if (first_frame)
	{
		prepareOldTexture(old_composite, command_buffer);
		prepareOldTexture(old_ssr, command_buffer);
		first_frame = false;
	}

	{
		ZoneScopedN("GBuffer pass");

		driver->setBlending(pipeline_state, false);
		driver->setCullMode(pipeline_state, render::backend::CullMode::BACK);
		driver->setDepthWrite(pipeline_state, true);
		driver->setDepthTest(pipeline_state, true);

		renderGBuffer(world, command_buffer, application_bindings, camera_bindings);
	}

	{
		ZoneScopedN("SSAO");

		driver->setBlending(pipeline_state, false);
		driver->setCullMode(pipeline_state, render::backend::CullMode::NONE);
		driver->setDepthWrite(pipeline_state, false);
		driver->setDepthTest(pipeline_state, false);

		renderSSAO(world, command_buffer, camera_bindings);
	}

	{
		ZoneScopedN("SSAO Blur");

		driver->setBlending(pipeline_state, false);
		driver->setCullMode(pipeline_state, render::backend::CullMode::NONE);
		driver->setDepthWrite(pipeline_state, false);
		driver->setDepthTest(pipeline_state, false);

		renderSSAOBlur(world, command_buffer, application_bindings);
	}

	{
		ZoneScopedN("LBuffer pass");

		renderLBuffer(world, command_buffer, camera_bindings);
	}

	{
		ZoneScopedN("SSR Trace");

		driver->setBlending(pipeline_state, false);
		driver->setCullMode(pipeline_state, render::backend::CullMode::NONE);
		driver->setDepthWrite(pipeline_state, false);
		driver->setDepthTest(pipeline_state, false);

		renderSSRTrace(world, command_buffer, application_bindings, camera_bindings);
	}

	{
		ZoneScopedN("SSR Resolve");

		driver->setBlending(pipeline_state, false);
		driver->setCullMode(pipeline_state, render::backend::CullMode::NONE);
		driver->setDepthWrite(pipeline_state, false);
		driver->setDepthTest(pipeline_state, false);

		renderSSRResolve(world, command_buffer, application_bindings, camera_bindings);
	}

	{
		ZoneScopedN("SSR Temporal Filter");

		driver->setBlending(pipeline_state, false);
		driver->setCullMode(pipeline_state, render::backend::CullMode::NONE);
		driver->setDepthWrite(pipeline_state, false);
		driver->setDepthTest(pipeline_state, false);

		renderSSRTemporalFilter(world, command_buffer);
	}

	{
		ZoneScopedN("Composite pass");

		renderComposite(world, command_buffer);
	}

	{
		ZoneScopedN("Composite Temporal Filter");

		driver->setBlending(pipeline_state, false);
		driver->setCullMode(pipeline_state, render::backend::CullMode::NONE);
		driver->setDepthWrite(pipeline_state, false);
		driver->setDepthTest(pipeline_state, false);

		renderCompositeTemporalFilter(world, command_buffer);
	}

	{
		ZoneScopedN("Tonemapping + ImGui pass");

		renderToSwapChain(world, command_buffer, swap_chain);
	}

	// Swap temporal filters
	std::swap(old_ssr, ssr);
	std::swap(old_composite, composite);

	driver->endCommandBuffer(command_buffer);
}

void RenderGraph::renderGBuffer(const game::World *world, render::backend::CommandBuffer *command_buffer, render::backend::BindSet *application_bindings, render::backend::BindSet *camera_bindings)
{
	driver->beginRenderPass(command_buffer, gbuffer_render_pass, gbuffer.frame_buffer);

	driver->clearPushConstants(pipeline_state);
	driver->clearBindSets(pipeline_state);
	driver->setBindSet(pipeline_state, 0, application_bindings);
	driver->setBindSet(pipeline_state, 1, camera_bindings);

	driver->clearShaders(pipeline_state);
	driver->setShader(pipeline_state, render::backend::ShaderType::VERTEX, gbuffer_pass_vertex->getBackend());
	driver->setShader(pipeline_state, render::backend::ShaderType::FRAGMENT, gbuffer_pass_fragment->getBackend());

	driver->clearVertexStreams(pipeline_state);

	ecs::render::drawRenderables(world, driver, 2, pipeline_state, command_buffer);

	driver->endRenderPass(command_buffer);
}

void RenderGraph::renderSSAO(const game::World *world, render::backend::CommandBuffer *command_buffer, render::backend::BindSet *camera_bindings)
{
	driver->beginRenderPass(command_buffer, ssao_render_pass, ssao_noised.frame_buffer);

	driver->clearPushConstants(pipeline_state);
	driver->clearBindSets(pipeline_state);
	driver->setBindSet(pipeline_state, 0, camera_bindings);
	driver->setBindSet(pipeline_state, 1, gbuffer.bindings);
	driver->setBindSet(pipeline_state, 2, ssao_kernel.bindings);

	driver->clearShaders(pipeline_state);
	driver->setShader(pipeline_state, render::backend::ShaderType::VERTEX, fullscreen_quad_vertex->getBackend());
	driver->setShader(pipeline_state, render::backend::ShaderType::FRAGMENT, ssao_pass_fragment->getBackend());

	driver->clearVertexStreams(pipeline_state);
	driver->setVertexStream(pipeline_state, 0, quad->getVertexBuffer());

	driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, quad->getIndexBuffer(), quad->getNumIndices());
	driver->endRenderPass(command_buffer);
}

void RenderGraph::renderSSAOBlur(const game::World *world, render::backend::CommandBuffer *command_buffer, render::backend::BindSet *application_bindings)
{
	driver->beginRenderPass(command_buffer, ssao_render_pass, ssao_blurred.frame_buffer);

	driver->clearPushConstants(pipeline_state);
	driver->clearBindSets(pipeline_state);
	driver->setBindSet(pipeline_state, 0, application_bindings);
	driver->setBindSet(pipeline_state, 1, ssao_noised.bindings);

	driver->clearShaders(pipeline_state);
	driver->setShader(pipeline_state, render::backend::ShaderType::VERTEX, fullscreen_quad_vertex->getBackend());
	driver->setShader(pipeline_state, render::backend::ShaderType::FRAGMENT, ssao_blur_pass_fragment->getBackend());

	driver->clearVertexStreams(pipeline_state);
	driver->setVertexStream(pipeline_state, 0, quad->getVertexBuffer());

	driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, quad->getIndexBuffer(), quad->getNumIndices());
	driver->endRenderPass(command_buffer);
}

void RenderGraph::renderSSRTrace(const game::World *world, render::backend::CommandBuffer *command_buffer, render::backend::BindSet *application_bindings, render::backend::BindSet *camera_bindings)
{
	driver->beginRenderPass(command_buffer, hdr_render_pass, ssr_trace.frame_buffer);

	driver->clearPushConstants(pipeline_state);
	driver->clearBindSets(pipeline_state);
	driver->setBindSet(pipeline_state, 0, application_bindings);
	driver->setBindSet(pipeline_state, 1, camera_bindings);
	driver->setBindSet(pipeline_state, 2, gbuffer.bindings);
	driver->setBindSet(pipeline_state, 3, ssr_data.bindings);

	driver->clearShaders(pipeline_state);
	driver->setShader(pipeline_state, render::backend::ShaderType::VERTEX, fullscreen_quad_vertex->getBackend());
	driver->setShader(pipeline_state, render::backend::ShaderType::FRAGMENT, ssr_trace_pass_fragment->getBackend());

	driver->clearVertexStreams(pipeline_state);
	driver->setVertexStream(pipeline_state, 0, quad->getVertexBuffer());

	driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, quad->getIndexBuffer(), quad->getNumIndices());
	driver->endRenderPass(command_buffer);
}

void RenderGraph::renderSSRResolve(const game::World *world, render::backend::CommandBuffer *command_buffer, render::backend::BindSet *application_bindings, render::backend::BindSet *camera_bindings)
{
	driver->beginRenderPass(command_buffer, ssr_resolve_render_pass, ssr_resolve.frame_buffer);

	driver->clearPushConstants(pipeline_state);
	driver->clearBindSets(pipeline_state);
	driver->setBindSet(pipeline_state, 0, application_bindings);
	driver->setBindSet(pipeline_state, 1, camera_bindings);
	driver->setBindSet(pipeline_state, 2, gbuffer.bindings);
	driver->setBindSet(pipeline_state, 3, ssr_trace.bindings);
	driver->setBindSet(pipeline_state, 4, ssr_data.bindings);
	driver->setBindSet(pipeline_state, 5, old_composite.bindings);

	driver->clearShaders(pipeline_state);
	driver->setShader(pipeline_state, render::backend::ShaderType::VERTEX, fullscreen_quad_vertex->getBackend());
	driver->setShader(pipeline_state, render::backend::ShaderType::FRAGMENT, ssr_resolve_pass_fragment->getBackend());

	driver->clearVertexStreams(pipeline_state);
	driver->setVertexStream(pipeline_state, 0, quad->getVertexBuffer());

	driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, quad->getIndexBuffer(), quad->getNumIndices());
	driver->endRenderPass(command_buffer);
}

void RenderGraph::renderSSRTemporalFilter(const game::World *world, render::backend::CommandBuffer *command_buffer)
{
	RenderBuffer ssr_resolve_temp { ssr_resolve.resolve, nullptr, ssr_resolve.resolve_bindings };
	RenderBuffer ssr_velocity_temp { ssr_resolve.velocity, nullptr, ssr_resolve.velocity_bindings };

	renderTemporalFilter(ssr, old_ssr, ssr_resolve_temp, ssr_velocity_temp, command_buffer);
}

void RenderGraph::renderLBuffer(const game::World *world, render::backend::CommandBuffer *command_buffer, render::backend::BindSet *camera_bindings)
{
	driver->beginRenderPass(command_buffer, lbuffer_render_pass, lbuffer.frame_buffer);

	driver->clearPushConstants(pipeline_state);
	driver->clearBindSets(pipeline_state);
	driver->setBindSet(pipeline_state, 0, camera_bindings);
	driver->setBindSet(pipeline_state, 1, gbuffer.bindings);

	ecs::render::drawSkyLights(world, driver, 2, pipeline_state, command_buffer);

	driver->endRenderPass(command_buffer);
}

void RenderGraph::renderComposite(const game::World *world, render::backend::CommandBuffer *command_buffer)
{
	driver->beginRenderPass(command_buffer, hdr_render_pass, composite_temp.frame_buffer);

	driver->clearPushConstants(pipeline_state);
	driver->clearBindSets(pipeline_state);
	driver->setBindSet(pipeline_state, 0, lbuffer.bindings);
	driver->setBindSet(pipeline_state, 1, ssao_blurred.bindings);
	driver->setBindSet(pipeline_state, 2, ssr.bindings);

	driver->clearShaders(pipeline_state);
	driver->setShader(pipeline_state, render::backend::ShaderType::VERTEX, fullscreen_quad_vertex->getBackend());
	driver->setShader(pipeline_state, render::backend::ShaderType::FRAGMENT, composite_pass_fragment->getBackend());

	// TODO: GI

	driver->clearVertexStreams(pipeline_state);
	driver->setVertexStream(pipeline_state, 0, quad->getVertexBuffer());

	driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, quad->getIndexBuffer(), quad->getNumIndices());
	driver->endRenderPass(command_buffer);
}

void RenderGraph::renderCompositeTemporalFilter(const game::World *world, render::backend::CommandBuffer *command_buffer)
{
	RenderBuffer gbuffer_velocity { gbuffer.velocity, nullptr, gbuffer.velocity_bindings };

	renderTemporalFilter(composite, old_composite, composite_temp, gbuffer_velocity, command_buffer);
}

void RenderGraph::renderTemporalFilter(RenderBuffer &current, const RenderBuffer &old, const RenderBuffer &temp, const RenderBuffer &velocity, render::backend::CommandBuffer *command_buffer)
{
	driver->beginRenderPass(command_buffer, hdr_render_pass, current.frame_buffer);

	driver->clearPushConstants(pipeline_state);
	driver->clearBindSets(pipeline_state);
	driver->setBindSet(pipeline_state, 0, temp.bindings);
	driver->setBindSet(pipeline_state, 1, old.bindings);
	driver->setBindSet(pipeline_state, 2, velocity.bindings);

	driver->clearShaders(pipeline_state);
	driver->setShader(pipeline_state, render::backend::ShaderType::VERTEX, fullscreen_quad_vertex->getBackend());
	driver->setShader(pipeline_state, render::backend::ShaderType::FRAGMENT, temporal_filter_pass_fragment->getBackend());

	driver->clearVertexStreams(pipeline_state);
	driver->setVertexStream(pipeline_state, 0, quad->getVertexBuffer());

	driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, quad->getIndexBuffer(), quad->getNumIndices());
	driver->endRenderPass(command_buffer);
}

void RenderGraph::renderToSwapChain(const game::World *world, render::backend::CommandBuffer *command_buffer, render::backend::SwapChain *swap_chain)
{
	if (swap_chain_render_pass == nullptr)
		swap_chain_render_pass = driver->createRenderPass(swap_chain, render::backend::RenderPassLoadOp::DONT_CARE, render::backend::RenderPassStoreOp::STORE, nullptr);

	driver->beginRenderPass(command_buffer, swap_chain_render_pass, swap_chain);

	driver->clearPushConstants(pipeline_state);
	driver->clearBindSets(pipeline_state);
	driver->setBindSet(pipeline_state, 0, composite.bindings);

	driver->clearShaders(pipeline_state);
	driver->setShader(pipeline_state, render::backend::ShaderType::VERTEX, fullscreen_quad_vertex->getBackend());
	driver->setShader(pipeline_state, render::backend::ShaderType::FRAGMENT, tonemapping_pass_fragment->getBackend());

	driver->clearVertexStreams(pipeline_state);
	driver->setVertexStream(pipeline_state, 0, quad->getVertexBuffer());

	driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, quad->getIndexBuffer(), quad->getNumIndices());

	imgui_renderer->render(command_buffer);

	driver->endRenderPass(command_buffer);
}

void RenderGraph::prepareOldTexture(const RenderBuffer &old, render::backend::CommandBuffer *command_buffer)
{
	driver->beginRenderPass(command_buffer, hdr_clear_render_pass, old.frame_buffer);

	driver->clearPushConstants(pipeline_state);
	driver->clearBindSets(pipeline_state);

	driver->clearShaders(pipeline_state);
	driver->setShader(pipeline_state, render::backend::ShaderType::VERTEX, fullscreen_quad_vertex->getBackend());

	driver->clearVertexStreams(pipeline_state);
	driver->setVertexStream(pipeline_state, 0, quad->getVertexBuffer());

	driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, quad->getIndexBuffer(), quad->getNumIndices());

	driver->endRenderPass(command_buffer);
}
