#include "RenderGraph.h"
#include "ApplicationResources.h"

#include "ImGuiRenderer.h"

#include <scapes/visual/API.h>
#include <scapes/visual/Resources.h>

#include <scapes/foundation/Profiler.h>

#include <imgui.h>

#include <cassert>

using namespace scapes;

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
	pipeline_state = device->createPipelineState();
	device->setViewport(pipeline_state, 0, 0, width, height);
	device->setScissor(pipeline_state, 0, 0, width, height);

	imgui_renderer = new ImGuiRenderer(device, visual_api);
	imgui_renderer->init(ImGui::GetCurrentContext());

	initRenderPasses();
	initSSAOKernel();
	initSSRData(resources->getBlueNoiseTexture());
	initTransient(width, height);

	fullscreen_quad = resources->getFullscreenQuad();

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

	device->destroyPipelineState(pipeline_state);
	pipeline_state = nullptr;

	delete imgui_renderer;
	imgui_renderer = nullptr;
}

void RenderGraph::initRenderPasses()
{
	foundation::render::Multisample samples = foundation::render::Multisample::COUNT_1;

	{ // GBuffer
		foundation::render::RenderPassClearValue clear_depth;
		clear_depth.as_depth_stencil = { 1.0f, 0 };

		foundation::render::RenderPassClearValue clear_color;
		clear_color.as_color = { 0.0f, 0.0f, 0.0f, 1.0f };

		foundation::render::RenderPassAttachment render_pass_attachments[5] =
		{
			{ foundation::render::Format::D32_SFLOAT, samples, foundation::render::RenderPassLoadOp::CLEAR, foundation::render::RenderPassStoreOp::STORE, clear_depth },
			{ foundation::render::Format::R8G8B8A8_UNORM, samples, foundation::render::RenderPassLoadOp::CLEAR, foundation::render::RenderPassStoreOp::STORE, clear_color },
			{ foundation::render::Format::R16G16B16A16_SFLOAT, samples, foundation::render::RenderPassLoadOp::CLEAR, foundation::render::RenderPassStoreOp::STORE, clear_color },
			{ foundation::render::Format::R8G8_UNORM, samples, foundation::render::RenderPassLoadOp::CLEAR, foundation::render::RenderPassStoreOp::STORE, clear_color },
			{ foundation::render::Format::R16G16_SFLOAT, samples, foundation::render::RenderPassLoadOp::CLEAR, foundation::render::RenderPassStoreOp::STORE, clear_color },
		};

		uint32_t color_attachments[4] = { 1, 2, 3, 4 };
		uint32_t depthstencil_attachment[1] = { 0 };

		foundation::render::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 4;
		render_pass_description.color_attachments = color_attachments;
		render_pass_description.depthstencil_attachment = depthstencil_attachment;

		gbuffer_render_pass = device->createRenderPass(5, render_pass_attachments, render_pass_description);
	}

	{ // LBuffer
		foundation::render::RenderPassAttachment render_pass_attachments[2] =
		{
			{ foundation::render::Format::R16G16B16A16_SFLOAT, samples, foundation::render::RenderPassLoadOp::DONT_CARE, foundation::render::RenderPassStoreOp::STORE },
			{ foundation::render::Format::R16G16B16A16_SFLOAT, samples, foundation::render::RenderPassLoadOp::DONT_CARE, foundation::render::RenderPassStoreOp::STORE },
		};

		uint32_t color_attachments[2] = { 0, 1 };

		foundation::render::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 2;
		render_pass_description.color_attachments = color_attachments;

		lbuffer_render_pass = device->createRenderPass(2, render_pass_attachments, render_pass_description);
	}

	{ // SSR Resolve
		foundation::render::RenderPassAttachment render_pass_attachments[2] =
		{
			{ foundation::render::Format::R16G16B16A16_SFLOAT, samples, foundation::render::RenderPassLoadOp::DONT_CARE, foundation::render::RenderPassStoreOp::STORE },
			{ foundation::render::Format::R16G16_SFLOAT, samples, foundation::render::RenderPassLoadOp::DONT_CARE, foundation::render::RenderPassStoreOp::STORE },
		};

		uint32_t color_attachments[2] = { 0, 1 };

		foundation::render::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 2;
		render_pass_description.color_attachments = color_attachments;

		ssr_resolve_render_pass = device->createRenderPass(2, render_pass_attachments, render_pass_description);
	}

	{ // SSAO
		foundation::render::RenderPassAttachment render_pass_attachments[1] =
		{
			{ foundation::render::Format::R8_UNORM, samples, foundation::render::RenderPassLoadOp::DONT_CARE, foundation::render::RenderPassStoreOp::STORE },
		};

		uint32_t color_attachments[1] = { 0 };

		foundation::render::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 1;
		render_pass_description.color_attachments = color_attachments;

		ssao_render_pass = device->createRenderPass(1, render_pass_attachments, render_pass_description);
	}

	{ // HDR
		foundation::render::RenderPassAttachment render_pass_attachments[1] =
		{
			{ foundation::render::Format::R16G16B16A16_SFLOAT, samples, foundation::render::RenderPassLoadOp::DONT_CARE, foundation::render::RenderPassStoreOp::STORE },
		};

		uint32_t color_attachments[1] = { 0 };

		foundation::render::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 1;
		render_pass_description.color_attachments = color_attachments;

		hdr_render_pass = device->createRenderPass(1, render_pass_attachments, render_pass_description);
	}

	{ // HDR Clear
		foundation::render::RenderPassClearValue clear_color;
		clear_color.as_color = { 0.0f, 0.0f, 0.0f, 0.0f };

		foundation::render::RenderPassAttachment render_pass_attachments[1] =
		{
			{ foundation::render::Format::R16G16B16A16_SFLOAT, samples, foundation::render::RenderPassLoadOp::CLEAR, foundation::render::RenderPassStoreOp::STORE, clear_color },
		};

		uint32_t color_attachments[1] = { 0 };

		foundation::render::RenderPassDescription render_pass_description = {};
		render_pass_description.num_color_attachments = 1;
		render_pass_description.color_attachments = color_attachments;

		hdr_clear_render_pass = device->createRenderPass(1, render_pass_attachments, render_pass_description);
	}
}

void RenderGraph::shutdownRenderPasses()
{
	device->destroyRenderPass(gbuffer_render_pass);
	gbuffer_render_pass = nullptr;

	device->destroyRenderPass(lbuffer_render_pass);
	lbuffer_render_pass = nullptr;

	device->destroyRenderPass(ssao_render_pass);
	ssao_render_pass = nullptr;

	device->destroyRenderPass(ssr_resolve_render_pass);
	ssr_resolve_render_pass = nullptr;

	device->destroyRenderPass(hdr_render_pass);
	hdr_render_pass = nullptr;

	device->destroyRenderPass(hdr_clear_render_pass);
	hdr_clear_render_pass = nullptr;

	device->destroyRenderPass(swap_chain_render_pass);
	swap_chain_render_pass = nullptr;
}

void RenderGraph::initTransient(uint32_t width, uint32_t height)
{
	initGBuffer(width, height);
	initLBuffer(width, height);
	initRenderBuffer(composite_temp, foundation::render::Format::R16G16B16A16_SFLOAT, width, height);
	initRenderBuffer(composite, foundation::render::Format::R16G16B16A16_SFLOAT, width, height);
	initRenderBuffer(old_composite, foundation::render::Format::R16G16B16A16_SFLOAT, width, height);

	initRenderBuffer(ssao_noised, foundation::render::Format::R8_UNORM, width, height);
	initRenderBuffer(ssao_blurred, foundation::render::Format::R8_UNORM, width, height);

	initRenderBuffer(ssr_trace, foundation::render::Format::R16G16B16A16_SFLOAT, width, height);
	initSSRResolve(ssr_resolve, width, height);
	initRenderBuffer(ssr, foundation::render::Format::R16G16B16A16_SFLOAT, width, height);
	initRenderBuffer(old_ssr, foundation::render::Format::R16G16B16A16_SFLOAT, width, height);

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
	gbuffer.base_color = device->createTexture2D(width, height, 1, foundation::render::Format::R8G8B8A8_UNORM);
	gbuffer.normal = device->createTexture2D(width, height, 1, foundation::render::Format::R16G16B16A16_SFLOAT);
	gbuffer.shading = device->createTexture2D(width, height, 1, foundation::render::Format::R8G8_UNORM);
	gbuffer.depth = device->createTexture2D(width, height, 1, foundation::render::Format::D32_SFLOAT);
	gbuffer.velocity = device->createTexture2D(width, height, 1, foundation::render::Format::R16G16_SFLOAT);

	foundation::render::FrameBufferAttachment gbuffer_attachments[5] = {
		{ gbuffer.depth },
		{ gbuffer.base_color },
		{ gbuffer.normal },
		{ gbuffer.shading },
		{ gbuffer.velocity },
	};

	gbuffer.frame_buffer = device->createFrameBuffer(5, gbuffer_attachments);
	gbuffer.bindings = device->createBindSet();

	device->bindTexture(gbuffer.bindings, 0, gbuffer.base_color);
	device->bindTexture(gbuffer.bindings, 1, gbuffer.normal);
	device->bindTexture(gbuffer.bindings, 2, gbuffer.shading);
	device->bindTexture(gbuffer.bindings, 3, gbuffer.depth);

	gbuffer.velocity_bindings = device->createBindSet();

	device->bindTexture(gbuffer.velocity_bindings, 0, gbuffer.velocity);
}

void RenderGraph::shutdownGBuffer()
{
	device->destroyTexture(gbuffer.base_color);
	device->destroyTexture(gbuffer.depth);
	device->destroyTexture(gbuffer.normal);
	device->destroyTexture(gbuffer.shading);
	device->destroyTexture(gbuffer.velocity);

	device->destroyFrameBuffer(gbuffer.frame_buffer);

	device->destroyBindSet(gbuffer.bindings);
	device->destroyBindSet(gbuffer.velocity_bindings);

	memset(&gbuffer, 0, sizeof(GBuffer));
}

void RenderGraph::initSSAOKernel()
{
	ssao_kernel.gpu_data = device->createUniformBuffer(foundation::render::BufferType::DYNAMIC, sizeof(SSAOKernel::CPUData));
	ssao_kernel.cpu_data = reinterpret_cast<SSAOKernel::CPUData *>(device->map(ssao_kernel.gpu_data));

	uint32_t data[SSAOKernel::MAX_NOISE_SAMPLES];
	for (int i = 0; i < SSAOKernel::MAX_NOISE_SAMPLES; ++i)
	{
		const foundation::math::vec3 &noise = foundation::math::normalize(foundation::math::vec3(randf(), randf(), 0.0f));
		data[i] = foundation::math::packHalf2x16(noise);
	}

	ssao_kernel.noise_texture = device->createTexture2D(4, 4, 1, foundation::render::Format::R16G16_SFLOAT, data);

	ssao_kernel.cpu_data->num_samples = 32;
	ssao_kernel.cpu_data->intensity = 1.5f;
	ssao_kernel.cpu_data->radius = 100.0f;

	buildSSAOKernel();

	ssao_kernel.bindings = device->createBindSet();

	device->bindUniformBuffer(ssao_kernel.bindings, 0, ssao_kernel.gpu_data);
	device->bindTexture(ssao_kernel.bindings, 1, ssao_kernel.noise_texture);
}

void RenderGraph::buildSSAOKernel()
{
	uint32_t num_samples = ssao_kernel.cpu_data->num_samples;
	float inum_samples = 1.0f / static_cast<float>(num_samples);

	for (uint32_t i = 0; i < num_samples; ++i)
	{
		foundation::math::vec4 sample;
		float radius = randf();
		float phi = foundation::math::radians(randf() * 360.0f);
		float theta = foundation::math::radians(randf() * 90.0f);

		float scale = i * inum_samples;
		scale = foundation::math::mix(0.1f, 1.0f, scale * scale);

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
	device->unmap(ssao_kernel.gpu_data);
	device->destroyUniformBuffer(ssao_kernel.gpu_data);
	device->destroyTexture(ssao_kernel.noise_texture);
	device->destroyBindSet(ssao_kernel.bindings);

	memset(&ssao_kernel, 0, sizeof(SSAOKernel));
}

void RenderGraph::initSSRData(visual::TextureHandle blue_noise)
{
	ssr_data.gpu_data = device->createUniformBuffer(foundation::render::BufferType::DYNAMIC, sizeof(SSRData::CPUData));
	ssr_data.cpu_data = reinterpret_cast<SSRData::CPUData *>(device->map(ssr_data.gpu_data));

	ssr_data.cpu_data->coarse_step_size = 100.0f;
	ssr_data.cpu_data->num_coarse_steps = 8;
	ssr_data.cpu_data->num_precision_steps = 8;
	ssr_data.cpu_data->facing_threshold = 0.0f;
	ssr_data.cpu_data->bypass_depth_threshold = 1.0f;

	ssr_data.bindings = device->createBindSet();

	device->bindUniformBuffer(ssr_data.bindings, 0, ssr_data.gpu_data);
	device->bindTexture(ssr_data.bindings, 1, blue_noise->gpu_data);
}

void RenderGraph::shutdownSSRData()
{
	device->unmap(ssr_data.gpu_data);
	device->destroyUniformBuffer(ssr_data.gpu_data);
	device->destroyBindSet(ssr_data.bindings);

	memset(&ssr_data, 0, sizeof(SSRData));
}

void RenderGraph::initRenderBuffer(RenderBuffer &ssr, foundation::render::Format format, uint32_t width, uint32_t height)
{
	ssr.texture = device->createTexture2D(width, height, 1, format);

	foundation::render::FrameBufferAttachment ssr_attachments[1] = {
		{ ssr.texture },
	};

	ssr.frame_buffer = device->createFrameBuffer(1, ssr_attachments);
	ssr.bindings = device->createBindSet();

	device->bindTexture(ssr.bindings, 0, ssr.texture);
}

void RenderGraph::shutdownRenderBuffer(RenderBuffer &ssr)
{
	device->destroyTexture(ssr.texture);
	device->destroyFrameBuffer(ssr.frame_buffer);
	device->destroyBindSet(ssr.bindings);

	memset(&ssr, 0, sizeof(RenderBuffer));
}

void RenderGraph::initSSRResolve(SSRResolve &ssr, uint32_t width, uint32_t height)
{
	ssr.resolve = device->createTexture2D(width, height, 1, foundation::render::Format::R16G16B16A16_SFLOAT);
	ssr.velocity = device->createTexture2D(width, height, 1, foundation::render::Format::R16G16_SFLOAT);

	foundation::render::FrameBufferAttachment ssr_attachments[2] = {
		{ ssr.resolve },
		{ ssr.velocity },
	};

	ssr.frame_buffer = device->createFrameBuffer(2, ssr_attachments);

	ssr.resolve_bindings = device->createBindSet();
	device->bindTexture(ssr.resolve_bindings, 0, ssr.resolve);

	ssr.velocity_bindings = device->createBindSet();
	device->bindTexture(ssr.velocity_bindings, 0, ssr.velocity);
}

void RenderGraph::shutdownSSRResolve(SSRResolve &ssr)
{
	device->destroyFrameBuffer(ssr.frame_buffer);
	device->destroyBindSet(ssr.resolve_bindings);
	device->destroyBindSet(ssr.velocity_bindings);
	device->destroyTexture(ssr.resolve);
	device->destroyTexture(ssr.velocity);

	memset(&ssr, 0, sizeof(SSRResolve));
}

void RenderGraph::initLBuffer(uint32_t width, uint32_t height)
{
	lbuffer.diffuse = device->createTexture2D(width, height, 1, foundation::render::Format::R16G16B16A16_SFLOAT);
	lbuffer.specular = device->createTexture2D(width, height, 1, foundation::render::Format::R16G16B16A16_SFLOAT);

	foundation::render::FrameBufferAttachment lbuffer_attachments[2] = {
		{ lbuffer.diffuse },
		{ lbuffer.specular },
	};

	lbuffer.frame_buffer = device->createFrameBuffer(2, lbuffer_attachments);
	lbuffer.bindings = device->createBindSet();

	device->bindTexture(lbuffer.bindings, 0, lbuffer.diffuse);
	device->bindTexture(lbuffer.bindings, 1, lbuffer.specular);
}

void RenderGraph::shutdownLBuffer()
{
	device->destroyTexture(lbuffer.diffuse);
	device->destroyTexture(lbuffer.specular);
	device->destroyFrameBuffer(lbuffer.frame_buffer);
	device->destroyBindSet(lbuffer.bindings);

	memset(&lbuffer, 0, sizeof(LBuffer));
}

void RenderGraph::resize(uint32_t width, uint32_t height)
{
	imgui_renderer->invalidateTextureIDs();

	shutdownTransient();
	initTransient(width, height);

	device->setViewport(pipeline_state, 0, 0, width, height);
	device->setScissor(pipeline_state, 0, 0, width, height);
}

/*
 */
ImTextureID RenderGraph::fetchTextureID(const foundation::render::Texture *texture)
{
	assert(imgui_renderer);

	return imgui_renderer->fetchTextureID(texture);
}

/*
 */
void RenderGraph::render(foundation::render::CommandBuffer *command_buffer, foundation::render::SwapChain *swap_chain, foundation::render::BindSet *application_bindings, foundation::render::BindSet *camera_bindings)
{
	device->resetCommandBuffer(command_buffer);
	device->beginCommandBuffer(command_buffer);

	if (first_frame)
	{
		prepareOldTexture(old_composite, command_buffer);
		prepareOldTexture(old_ssr, command_buffer);
		first_frame = false;
	}

	{
		SCAPES_PROFILER_SCOPED_N("GBuffer pass");

		device->setBlending(pipeline_state, false);
		device->setCullMode(pipeline_state, foundation::render::CullMode::BACK);
		device->setDepthWrite(pipeline_state, true);
		device->setDepthTest(pipeline_state, true);

		renderGBuffer(command_buffer, application_bindings, camera_bindings);
	}

	{
		SCAPES_PROFILER_SCOPED_N("SSAO");

		device->setBlending(pipeline_state, false);
		device->setCullMode(pipeline_state, foundation::render::CullMode::NONE);
		device->setDepthWrite(pipeline_state, false);
		device->setDepthTest(pipeline_state, false);

		renderSSAO(command_buffer, camera_bindings);
	}

	{
		SCAPES_PROFILER_SCOPED_N("SSAO Blur");

		device->setBlending(pipeline_state, false);
		device->setCullMode(pipeline_state, foundation::render::CullMode::NONE);
		device->setDepthWrite(pipeline_state, false);
		device->setDepthTest(pipeline_state, false);

		renderSSAOBlur(command_buffer, application_bindings);
	}

	{
		SCAPES_PROFILER_SCOPED_N("LBuffer pass");

		renderLBuffer(command_buffer, camera_bindings);
	}

	{
		SCAPES_PROFILER_SCOPED_N("SSR Trace");

		device->setBlending(pipeline_state, false);
		device->setCullMode(pipeline_state, foundation::render::CullMode::NONE);
		device->setDepthWrite(pipeline_state, false);
		device->setDepthTest(pipeline_state, false);

		renderSSRTrace(command_buffer, application_bindings, camera_bindings);
	}

	{
		SCAPES_PROFILER_SCOPED_N("SSR Resolve");

		device->setBlending(pipeline_state, false);
		device->setCullMode(pipeline_state, foundation::render::CullMode::NONE);
		device->setDepthWrite(pipeline_state, false);
		device->setDepthTest(pipeline_state, false);

		renderSSRResolve(command_buffer, application_bindings, camera_bindings);
	}

	{
		SCAPES_PROFILER_SCOPED_N("SSR Temporal Filter");

		device->setBlending(pipeline_state, false);
		device->setCullMode(pipeline_state, foundation::render::CullMode::NONE);
		device->setDepthWrite(pipeline_state, false);
		device->setDepthTest(pipeline_state, false);

		renderSSRTemporalFilter(command_buffer);
	}

	{
		SCAPES_PROFILER_SCOPED_N("Composite pass");

		renderComposite(command_buffer);
	}

	{
		SCAPES_PROFILER_SCOPED_N("Composite Temporal Filter");

		device->setBlending(pipeline_state, false);
		device->setCullMode(pipeline_state, foundation::render::CullMode::NONE);
		device->setDepthWrite(pipeline_state, false);
		device->setDepthTest(pipeline_state, false);

		renderCompositeTemporalFilter(command_buffer);
	}

	{
		SCAPES_PROFILER_SCOPED_N("Tonemapping + ImGui pass");

		renderToSwapChain(command_buffer, swap_chain);
	}

	// Swap temporal filters
	std::swap(old_ssr, ssr);
	std::swap(old_composite, composite);

	device->endCommandBuffer(command_buffer);
}

void RenderGraph::renderGBuffer(foundation::render::CommandBuffer *command_buffer, foundation::render::BindSet *application_bindings, foundation::render::BindSet *camera_bindings)
{
	device->beginRenderPass(command_buffer, gbuffer_render_pass, gbuffer.frame_buffer);

	device->clearPushConstants(pipeline_state);
	device->clearBindSets(pipeline_state);
	device->setBindSet(pipeline_state, 0, application_bindings);
	device->setBindSet(pipeline_state, 1, camera_bindings);

	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, gbuffer_pass_vertex->shader);
	device->setShader(pipeline_state, foundation::render::ShaderType::FRAGMENT, gbuffer_pass_fragment->shader);

	device->clearVertexStreams(pipeline_state);

	visual_api->drawRenderables(2, pipeline_state, command_buffer);

	device->endRenderPass(command_buffer);
}

void RenderGraph::renderSSAO(foundation::render::CommandBuffer *command_buffer, foundation::render::BindSet *camera_bindings)
{
	device->beginRenderPass(command_buffer, ssao_render_pass, ssao_noised.frame_buffer);

	device->clearPushConstants(pipeline_state);
	device->clearBindSets(pipeline_state);
	device->setBindSet(pipeline_state, 0, camera_bindings);
	device->setBindSet(pipeline_state, 1, gbuffer.bindings);
	device->setBindSet(pipeline_state, 2, ssao_kernel.bindings);

	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, fullscreen_quad_vertex->shader);
	device->setShader(pipeline_state, foundation::render::ShaderType::FRAGMENT, ssao_pass_fragment->shader);

	device->clearVertexStreams(pipeline_state);
	device->setVertexStream(pipeline_state, 0, fullscreen_quad->vertex_buffer);

	device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, fullscreen_quad->index_buffer, fullscreen_quad->num_indices);
	device->endRenderPass(command_buffer);
}

void RenderGraph::renderSSAOBlur(foundation::render::CommandBuffer *command_buffer, foundation::render::BindSet *application_bindings)
{
	device->beginRenderPass(command_buffer, ssao_render_pass, ssao_blurred.frame_buffer);

	device->clearPushConstants(pipeline_state);
	device->clearBindSets(pipeline_state);
	device->setBindSet(pipeline_state, 0, application_bindings);
	device->setBindSet(pipeline_state, 1, ssao_noised.bindings);

	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, fullscreen_quad_vertex->shader);
	device->setShader(pipeline_state, foundation::render::ShaderType::FRAGMENT, ssao_blur_pass_fragment->shader);

	device->clearVertexStreams(pipeline_state);
	device->setVertexStream(pipeline_state, 0, fullscreen_quad->vertex_buffer);

	device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, fullscreen_quad->index_buffer, fullscreen_quad->num_indices);
	device->endRenderPass(command_buffer);
}

void RenderGraph::renderSSRTrace(foundation::render::CommandBuffer *command_buffer, foundation::render::BindSet *application_bindings, foundation::render::BindSet *camera_bindings)
{
	device->beginRenderPass(command_buffer, hdr_render_pass, ssr_trace.frame_buffer);

	device->clearPushConstants(pipeline_state);
	device->clearBindSets(pipeline_state);
	device->setBindSet(pipeline_state, 0, application_bindings);
	device->setBindSet(pipeline_state, 1, camera_bindings);
	device->setBindSet(pipeline_state, 2, gbuffer.bindings);
	device->setBindSet(pipeline_state, 3, ssr_data.bindings);

	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, fullscreen_quad_vertex->shader);
	device->setShader(pipeline_state, foundation::render::ShaderType::FRAGMENT, ssr_trace_pass_fragment->shader);

	device->clearVertexStreams(pipeline_state);
	device->setVertexStream(pipeline_state, 0, fullscreen_quad->vertex_buffer);

	device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, fullscreen_quad->index_buffer, fullscreen_quad->num_indices);
	device->endRenderPass(command_buffer);
}

void RenderGraph::renderSSRResolve(foundation::render::CommandBuffer *command_buffer, foundation::render::BindSet *application_bindings, foundation::render::BindSet *camera_bindings)
{
	device->beginRenderPass(command_buffer, ssr_resolve_render_pass, ssr_resolve.frame_buffer);

	device->clearPushConstants(pipeline_state);
	device->clearBindSets(pipeline_state);
	device->setBindSet(pipeline_state, 0, application_bindings);
	device->setBindSet(pipeline_state, 1, camera_bindings);
	device->setBindSet(pipeline_state, 2, gbuffer.bindings);
	device->setBindSet(pipeline_state, 3, ssr_trace.bindings);
	device->setBindSet(pipeline_state, 4, ssr_data.bindings);
	device->setBindSet(pipeline_state, 5, old_composite.bindings);

	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, fullscreen_quad_vertex->shader);
	device->setShader(pipeline_state, foundation::render::ShaderType::FRAGMENT, ssr_resolve_pass_fragment->shader);

	device->clearVertexStreams(pipeline_state);
	device->setVertexStream(pipeline_state, 0, fullscreen_quad->vertex_buffer);

	device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, fullscreen_quad->index_buffer, fullscreen_quad->num_indices);
	device->endRenderPass(command_buffer);
}

void RenderGraph::renderSSRTemporalFilter(foundation::render::CommandBuffer *command_buffer)
{
	RenderBuffer ssr_resolve_temp { ssr_resolve.resolve, nullptr, ssr_resolve.resolve_bindings };
	RenderBuffer ssr_velocity_temp { ssr_resolve.velocity, nullptr, ssr_resolve.velocity_bindings };

	renderTemporalFilter(ssr, old_ssr, ssr_resolve_temp, ssr_velocity_temp, command_buffer);
}

void RenderGraph::renderLBuffer(foundation::render::CommandBuffer *command_buffer, foundation::render::BindSet *camera_bindings)
{
	device->beginRenderPass(command_buffer, lbuffer_render_pass, lbuffer.frame_buffer);

	device->clearPushConstants(pipeline_state);
	device->clearBindSets(pipeline_state);
	device->setBindSet(pipeline_state, 0, camera_bindings);
	device->setBindSet(pipeline_state, 1, gbuffer.bindings);

	visual_api->drawSkyLights(2, pipeline_state, command_buffer);

	device->endRenderPass(command_buffer);
}

void RenderGraph::renderComposite(foundation::render::CommandBuffer *command_buffer)
{
	device->beginRenderPass(command_buffer, hdr_render_pass, composite_temp.frame_buffer);

	device->clearPushConstants(pipeline_state);
	device->clearBindSets(pipeline_state);
	device->setBindSet(pipeline_state, 0, lbuffer.bindings);
	device->setBindSet(pipeline_state, 1, ssao_blurred.bindings);
	device->setBindSet(pipeline_state, 2, ssr.bindings);

	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, fullscreen_quad_vertex->shader);
	device->setShader(pipeline_state, foundation::render::ShaderType::FRAGMENT, composite_pass_fragment->shader);

	// TODO: GI

	device->clearVertexStreams(pipeline_state);
	device->setVertexStream(pipeline_state, 0, fullscreen_quad->vertex_buffer);

	device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, fullscreen_quad->index_buffer, fullscreen_quad->num_indices);
	device->endRenderPass(command_buffer);
}

void RenderGraph::renderCompositeTemporalFilter(foundation::render::CommandBuffer *command_buffer)
{
	RenderBuffer gbuffer_velocity { gbuffer.velocity, nullptr, gbuffer.velocity_bindings };

	renderTemporalFilter(composite, old_composite, composite_temp, gbuffer_velocity, command_buffer);
}

void RenderGraph::renderTemporalFilter(RenderBuffer &current, const RenderBuffer &old, const RenderBuffer &temp, const RenderBuffer &velocity, foundation::render::CommandBuffer *command_buffer)
{
	device->beginRenderPass(command_buffer, hdr_render_pass, current.frame_buffer);

	device->clearPushConstants(pipeline_state);
	device->clearBindSets(pipeline_state);
	device->setBindSet(pipeline_state, 0, temp.bindings);
	device->setBindSet(pipeline_state, 1, old.bindings);
	device->setBindSet(pipeline_state, 2, velocity.bindings);

	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, fullscreen_quad_vertex->shader);
	device->setShader(pipeline_state, foundation::render::ShaderType::FRAGMENT, temporal_filter_pass_fragment->shader);

	device->clearVertexStreams(pipeline_state);
	device->setVertexStream(pipeline_state, 0, fullscreen_quad->vertex_buffer);

	device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, fullscreen_quad->index_buffer, fullscreen_quad->num_indices);
	device->endRenderPass(command_buffer);
}

void RenderGraph::renderToSwapChain(foundation::render::CommandBuffer *command_buffer, foundation::render::SwapChain *swap_chain)
{
	if (swap_chain_render_pass == nullptr)
		swap_chain_render_pass = device->createRenderPass(swap_chain, foundation::render::RenderPassLoadOp::DONT_CARE, foundation::render::RenderPassStoreOp::STORE, nullptr);

	device->beginRenderPass(command_buffer, swap_chain_render_pass, swap_chain);

	device->clearPushConstants(pipeline_state);
	device->clearBindSets(pipeline_state);
	device->setBindSet(pipeline_state, 0, composite.bindings);

	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, fullscreen_quad_vertex->shader);
	device->setShader(pipeline_state, foundation::render::ShaderType::FRAGMENT, tonemapping_pass_fragment->shader);

	device->clearVertexStreams(pipeline_state);
	device->setVertexStream(pipeline_state, 0, fullscreen_quad->vertex_buffer);

	device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, fullscreen_quad->index_buffer, fullscreen_quad->num_indices);

	imgui_renderer->render(command_buffer);

	device->endRenderPass(command_buffer);
}

void RenderGraph::prepareOldTexture(const RenderBuffer &old, foundation::render::CommandBuffer *command_buffer)
{
	device->beginRenderPass(command_buffer, hdr_clear_render_pass, old.frame_buffer);

	device->clearPushConstants(pipeline_state);
	device->clearBindSets(pipeline_state);

	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, fullscreen_quad_vertex->shader);

	device->clearVertexStreams(pipeline_state);
	device->setVertexStream(pipeline_state, 0, fullscreen_quad->vertex_buffer);

	device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, fullscreen_quad->index_buffer, fullscreen_quad->num_indices);

	device->endRenderPass(command_buffer);
}
