#pragma once

#include <scapes/visual/Fwd.h>

#include <render/backend/Driver.h>
#include <common/Math.h>

class ApplicationResources;
struct Shader;
struct Mesh;
struct Texture;
struct RenderFrame;

class ImGuiRenderer;
typedef void* ImTextureID;

namespace game
{
	class World;
}

namespace render::shaders
{
	class Compiler;
}

struct GBuffer
{
	render::backend::Texture *base_color {nullptr};     // rgba8, rga - albedo for dielectrics, f0 for conductors, a - unused
	render::backend::Texture *shading {nullptr};        // rg8,   r - roughness, g - metalness
	render::backend::Texture *normal {nullptr};         // rg16f, packed normal without z
	render::backend::Texture *depth {nullptr};          // d32f,  linear depth
	render::backend::Texture *velocity {nullptr};       // rg16f, uv motion vector
	render::backend::FrameBuffer *frame_buffer {nullptr};
	render::backend::BindSet *bindings {nullptr};
	render::backend::BindSet *velocity_bindings {nullptr};
};

struct LBuffer
{
	render::backend::Texture *diffuse {nullptr};        // rgb16f, hdr linear color
	render::backend::Texture *specular {nullptr};       // rga16f, hdr linear color
	render::backend::FrameBuffer *frame_buffer {nullptr};
	render::backend::BindSet *bindings {nullptr};
};

struct SSAOKernel
{
	enum
	{
		MAX_SAMPLES = 256,
		MAX_NOISE_SAMPLES = 16,
	};

	struct CPUData
	{
		uint32_t num_samples {32};
		float radius {1.0f};
		float intensity {1.0f};
		float alignment[1]; // we need 16 byte alignment
		glm::vec4 samples[MAX_SAMPLES];
	};

	CPUData *cpu_data {nullptr};
	render::backend::UniformBuffer *gpu_data {nullptr};
	render::backend::Texture *noise_texture {nullptr}; // rg16f, random basis rotation in screen-space
	render::backend::BindSet *bindings {nullptr};
};

struct SSRData
{
	struct CPUData
	{
		float coarse_step_size {1.0f};
		uint32_t num_coarse_steps {8};
		uint32_t num_precision_steps {8};
		float facing_threshold {0.0f};
		float bypass_depth_threshold {0.5f};
	};

	CPUData *cpu_data {nullptr};
	render::backend::UniformBuffer *gpu_data {nullptr};
	render::backend::BindSet *bindings {nullptr};
};

struct SSRResolve
{
	render::backend::Texture *resolve {nullptr};   // rgba16f, hdr indirect specular linear color
	render::backend::Texture *velocity {nullptr};  // rg16f, reprojected reflected surface motion vectors
	render::backend::BindSet *resolve_bindings {nullptr};
	render::backend::BindSet *velocity_bindings {nullptr};
	render::backend::FrameBuffer *frame_buffer {nullptr};
};

// SSAO, r8, ao factor
// SSR trace, rgba16f, trace data
// Composite, rgba16f, hdr linear color
struct RenderBuffer
{
	render::backend::Texture *texture {nullptr};
	render::backend::FrameBuffer *frame_buffer {nullptr};
	render::backend::BindSet *bindings {nullptr};
};

/*
 */
class RenderGraph
{
public:
	RenderGraph(render::backend::Driver *driver, scapes::visual::API *visual_api)
		: driver(driver), visual_api(visual_api) { }

	virtual ~RenderGraph();

	void init(const ApplicationResources *resources, uint32_t width, uint32_t height);
	void shutdown();
	void resize(uint32_t width, uint32_t height);
	void render(render::backend::CommandBuffer *command_buffer, render::backend::SwapChain *swap_chain, render::backend::BindSet *application_bindings, render::backend::BindSet *camera_bindings);

	ImTextureID fetchTextureID(const render::backend::Texture *texture);

	const GBuffer &getGBuffer() const { return gbuffer; }
	const LBuffer &getLBuffer() const { return lbuffer; }

	const SSAOKernel &getSSAOKernel() const { return ssao_kernel; }
	SSAOKernel &getSSAOKernel() { return ssao_kernel; }

	const RenderBuffer &getSSAONoised() const { return ssao_noised; }
	const RenderBuffer &getSSAOBlurred() const { return ssao_blurred; }

	const SSRData &getSSRData() const { return ssr_data; }
	SSRData &getSSRData() { return ssr_data; }

	const RenderBuffer &getSSRTrace() const { return ssr_trace; }

	void buildSSAOKernel();

private:
	void initRenderPasses();
	void shutdownRenderPasses();

	void initTransient(uint32_t width, uint32_t height);
	void shutdownTransient();

	void initGBuffer(uint32_t width, uint32_t height);
	void shutdownGBuffer();

	void initSSAOKernel();
	void shutdownSSAOKernel();

	void initSSRData(scapes::visual::TextureHandle blue_noise);
	void shutdownSSRData();

	void initRenderBuffer(RenderBuffer &ssr, render::backend::Format format, uint32_t width, uint32_t height);
	void shutdownRenderBuffer(RenderBuffer &ssr);

	void initSSRResolve(SSRResolve &ssr, uint32_t width, uint32_t height);
	void shutdownSSRResolve(SSRResolve &ssr);

	void initLBuffer(uint32_t width, uint32_t height);
	void shutdownLBuffer();

	void renderGBuffer(render::backend::CommandBuffer *command_buffer, render::backend::BindSet *application_bindings, render::backend::BindSet *camera_bindings);
	void renderSSAO(render::backend::CommandBuffer *command_buffer, render::backend::BindSet *camera_bindings);
	void renderSSAOBlur(render::backend::CommandBuffer *command_buffer, render::backend::BindSet *application_bindings);
	void renderSSRTrace(render::backend::CommandBuffer *command_buffer, render::backend::BindSet *application_bindings, render::backend::BindSet *camera_bindings);
	void renderSSRResolve(render::backend::CommandBuffer *command_buffer, render::backend::BindSet *application_bindings, render::backend::BindSet *camera_bindings);
	void renderSSRTemporalFilter(render::backend::CommandBuffer *command_buffer);
	void renderLBuffer(render::backend::CommandBuffer *command_buffer, render::backend::BindSet *camera_bindings);
	void renderComposite(render::backend::CommandBuffer *command_buffer);
	void renderCompositeTemporalFilter(render::backend::CommandBuffer *command_buffer);
	void renderTemporalFilter(RenderBuffer &current, const RenderBuffer &old, const RenderBuffer &temp, const RenderBuffer &velocity, render::backend::CommandBuffer *command_buffer);
	void renderToSwapChain(render::backend::CommandBuffer *command_buffer, render::backend::SwapChain *swap_chain);

	void prepareOldTexture(const RenderBuffer &old, render::backend::CommandBuffer *command_buffer);

private:
	render::backend::Driver *driver {nullptr};
	render::backend::PipelineState *pipeline_state {nullptr};
	scapes::visual::API *visual_api {nullptr};

	GBuffer gbuffer;

	SSAOKernel ssao_kernel;
	RenderBuffer ssao_noised;
	RenderBuffer ssao_blurred;

	SSRData ssr_data;
	RenderBuffer ssr_trace;
	SSRResolve ssr_resolve;

	RenderBuffer ssr;
	RenderBuffer old_ssr;

	LBuffer lbuffer;
	RenderBuffer composite_temp;
	RenderBuffer composite;
	RenderBuffer old_composite;

	bool first_frame {true};

	ImGuiRenderer *imgui_renderer {nullptr};

	render::backend::RenderPass *gbuffer_render_pass {nullptr};
	render::backend::RenderPass *lbuffer_render_pass {nullptr};
	render::backend::RenderPass *ssr_resolve_render_pass {nullptr};
	render::backend::RenderPass *ssao_render_pass {nullptr};
	render::backend::RenderPass *hdr_render_pass {nullptr};
	render::backend::RenderPass *hdr_clear_render_pass {nullptr};
	render::backend::RenderPass *swap_chain_render_pass {nullptr};

	scapes::visual::MeshHandle fullscreen_quad;

	scapes::visual::ShaderHandle gbuffer_pass_vertex;
	scapes::visual::ShaderHandle gbuffer_pass_fragment;

	scapes::visual::ShaderHandle fullscreen_quad_vertex;

	scapes::visual::ShaderHandle ssao_pass_fragment;
	scapes::visual::ShaderHandle ssao_blur_pass_fragment;
	scapes::visual::ShaderHandle ssr_trace_pass_fragment;
	scapes::visual::ShaderHandle ssr_resolve_pass_fragment;
	scapes::visual::ShaderHandle temporal_filter_pass_fragment;
	scapes::visual::ShaderHandle composite_pass_fragment;
	scapes::visual::ShaderHandle tonemapping_pass_fragment;
};
