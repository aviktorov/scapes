#pragma once

#include <scapes/foundation/math/Math.h>
#include <scapes/foundation/resources/ResourceManager.h>

#include <scapes/visual/Fwd.h>

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
	scapes::foundation::render::Texture *base_color {nullptr};     // rgba8, rga - albedo for dielectrics, f0 for conductors, a - unused
	scapes::foundation::render::Texture *shading {nullptr};        // rg8,   r - roughness, g - metalness
	scapes::foundation::render::Texture *normal {nullptr};         // rg16f, packed normal without z
	scapes::foundation::render::Texture *depth {nullptr};          // d32f,  linear depth
	scapes::foundation::render::Texture *velocity {nullptr};       // rg16f, uv motion vector
	scapes::foundation::render::FrameBuffer *frame_buffer {nullptr};
	scapes::foundation::render::BindSet *bindings {nullptr};
	scapes::foundation::render::BindSet *velocity_bindings {nullptr};
};

struct LBuffer
{
	scapes::foundation::render::Texture *diffuse {nullptr};        // rgb16f, hdr linear color
	scapes::foundation::render::Texture *specular {nullptr};       // rga16f, hdr linear color
	scapes::foundation::render::FrameBuffer *frame_buffer {nullptr};
	scapes::foundation::render::BindSet *bindings {nullptr};
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
		scapes::foundation::math::vec4 samples[MAX_SAMPLES];
	};

	CPUData *cpu_data {nullptr};
	scapes::foundation::render::UniformBuffer *gpu_data {nullptr};
	scapes::foundation::render::Texture *noise_texture {nullptr}; // rg16f, random basis rotation in screen-space
	scapes::foundation::render::BindSet *bindings {nullptr};
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
	scapes::foundation::render::UniformBuffer *gpu_data {nullptr};
	scapes::foundation::render::BindSet *bindings {nullptr};
};

struct SSRResolve
{
	scapes::foundation::render::Texture *resolve {nullptr};   // rgba16f, hdr indirect specular linear color
	scapes::foundation::render::Texture *velocity {nullptr};  // rg16f, reprojected reflected surface motion vectors
	scapes::foundation::render::BindSet *resolve_bindings {nullptr};
	scapes::foundation::render::BindSet *velocity_bindings {nullptr};
	scapes::foundation::render::FrameBuffer *frame_buffer {nullptr};
};

// SSAO, r8, ao factor
// SSR trace, rgba16f, trace data
// Composite, rgba16f, hdr linear color
struct RenderBuffer
{
	scapes::foundation::render::Texture *texture {nullptr};
	scapes::foundation::render::FrameBuffer *frame_buffer {nullptr};
	scapes::foundation::render::BindSet *bindings {nullptr};
};

/*
 */
class RenderGraph
{
public:
	RenderGraph(scapes::foundation::render::Device *device, scapes::visual::API *visual_api)
		: device(device), visual_api(visual_api) { }

	virtual ~RenderGraph();

	void init(const ApplicationResources *resources, uint32_t width, uint32_t height);
	void shutdown();
	void resize(uint32_t width, uint32_t height);
	void render(
		scapes::foundation::render::CommandBuffer *command_buffer,
		scapes::foundation::render::SwapChain *swap_chain,
		scapes::foundation::render::BindSet *application_bindings,
		scapes::foundation::render::BindSet *camera_bindings
	);

	ImTextureID fetchTextureID(const scapes::foundation::render::Texture *texture);

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

	void initRenderBuffer(RenderBuffer &ssr, scapes::foundation::render::Format format, uint32_t width, uint32_t height);
	void shutdownRenderBuffer(RenderBuffer &ssr);

	void initSSRResolve(SSRResolve &ssr, uint32_t width, uint32_t height);
	void shutdownSSRResolve(SSRResolve &ssr);

	void initLBuffer(uint32_t width, uint32_t height);
	void shutdownLBuffer();

	void renderGBuffer(
		scapes::foundation::render::CommandBuffer *command_buffer,
		scapes::foundation::render::BindSet *application_bindings,
		scapes::foundation::render::BindSet *camera_bindings
	);

	void renderSSAO(
		scapes::foundation::render::CommandBuffer *command_buffer,
		scapes::foundation::render::BindSet *camera_bindings
	);

	void renderSSAOBlur(
		scapes::foundation::render::CommandBuffer *command_buffer,
		scapes::foundation::render::BindSet *application_bindings
	);

	void renderSSRTrace(
		scapes::foundation::render::CommandBuffer *command_buffer,
		scapes::foundation::render::BindSet *application_bindings,
		scapes::foundation::render::BindSet *camera_bindings
	);

	void renderSSRResolve(
		scapes::foundation::render::CommandBuffer *command_buffer,
		scapes::foundation::render::BindSet *application_bindings,
		scapes::foundation::render::BindSet *camera_bindings
	);

	void renderSSRTemporalFilter(
		scapes::foundation::render::CommandBuffer *command_buffer
	);

	void renderLBuffer(
		scapes::foundation::render::CommandBuffer *command_buffer,
		scapes::foundation::render::BindSet *camera_bindings
	);

	void renderComposite(
		scapes::foundation::render::CommandBuffer *command_buffer
	);

	void renderCompositeTemporalFilter(
		scapes::foundation::render::CommandBuffer *command_buffer
	);

	void renderTemporalFilter(
		RenderBuffer &current,
		const RenderBuffer &old,
		const RenderBuffer &temp,
		const RenderBuffer &velocity,
		scapes::foundation::render::CommandBuffer *command_buffer
	);

	void renderToSwapChain(
		scapes::foundation::render::CommandBuffer *command_buffer,
		scapes::foundation::render::SwapChain *swap_chain
	);

	void prepareOldTexture(
		const RenderBuffer &old,
		scapes::foundation::render::CommandBuffer *command_buffer
	);

private:
	scapes::foundation::render::Device *device {nullptr};
	scapes::foundation::render::PipelineState *pipeline_state {nullptr};
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

	scapes::foundation::render::RenderPass *gbuffer_render_pass {nullptr};
	scapes::foundation::render::RenderPass *lbuffer_render_pass {nullptr};
	scapes::foundation::render::RenderPass *ssr_resolve_render_pass {nullptr};
	scapes::foundation::render::RenderPass *ssao_render_pass {nullptr};
	scapes::foundation::render::RenderPass *hdr_render_pass {nullptr};
	scapes::foundation::render::RenderPass *hdr_clear_render_pass {nullptr};
	scapes::foundation::render::RenderPass *swap_chain_render_pass {nullptr};

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
