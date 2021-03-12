#pragma once

#include <render/backend/driver.h>
#include <glm/vec4.hpp>

class ApplicationResources;
class Scene;
class ImGuiRenderer;

typedef void* ImTextureID;

namespace render
{
	class Shader;
	class Mesh;
	class Texture;
	struct RenderFrame;
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
	render::backend::FrameBuffer *framebuffer {nullptr};
	render::backend::BindSet *bindings {nullptr};
};

struct LBuffer
{
	render::backend::Texture *diffuse {nullptr};        // rgb16f, hdr linear color
	render::backend::Texture *specular {nullptr};       // rga16f, hdr linear color
	render::backend::FrameBuffer *framebuffer {nullptr};
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

// SSAO, r8, ao factor
// SSR trace, rgba8, trace data
// SSR resolve, rgba16f, indirect specular
// Composite, rgba16f, hdr linear color
struct RenderBuffer
{
	render::backend::Texture *texture {nullptr};
	render::backend::FrameBuffer *framebuffer {nullptr};
	render::backend::BindSet *bindings {nullptr};
};

/*
 */
class RenderGraph
{
public:
	RenderGraph(render::backend::Driver *driver, render::shaders::Compiler *compiler)
		: driver(driver), compiler(compiler) { }

	virtual ~RenderGraph();

	void init(const ApplicationResources *resources, uint32_t width, uint32_t height);
	void shutdown();
	void resize(uint32_t width, uint32_t height);
	void render(const Scene *scene, const render::RenderFrame &frame);

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
	void initGBuffer(uint32_t width, uint32_t height);
	void shutdownGBuffer();

	void initSSAOKernel();
	void shutdownSSAOKernel();

	void initSSAO(RenderBuffer &ssao, uint32_t width, uint32_t height);
	void shutdownSSAO(RenderBuffer &ssao);

	void initSSRData(const render::Texture *blue_noise);
	void shutdownSSRData();

	void initSSR(RenderBuffer &ssr, render::backend::Format format, uint32_t width, uint32_t height);
	void shutdownSSR(RenderBuffer &ssr);

	void initLBuffer(uint32_t width, uint32_t height);
	void shutdownLBuffer();

	void initComposite(RenderBuffer &composite, uint32_t width, uint32_t height);
	void shutdownComposite(RenderBuffer &composite);

	void renderGBuffer(const Scene *scene, const render::RenderFrame &frame);
	void renderSSAO(const Scene *scene, const render::RenderFrame &frame);
	void renderSSAOBlur(const Scene *scene, const render::RenderFrame &frame);
	void renderSSRTrace(const Scene *scene, const render::RenderFrame &frame);
	void renderSSRResolve(const Scene *scene, const render::RenderFrame &frame);
	void renderSSRTemporalFilter(const Scene *scene, const render::RenderFrame &frame);
	void renderLBuffer(const Scene *scene, const render::RenderFrame &frame);
	void renderComposite(const Scene *scene, const render::RenderFrame &frame);
	void renderCompositeTemporalFilter(const Scene *scene, const render::RenderFrame &frame);
	void renderTemporalFilter(RenderBuffer &current, const RenderBuffer &old, const RenderBuffer &temp, const render::RenderFrame &frame);
	void renderFinal(const Scene *scene, const render::RenderFrame &frame);

private:
	render::backend::Driver *driver {nullptr};
	render::shaders::Compiler *compiler {nullptr};

	GBuffer gbuffer;

	SSAOKernel ssao_kernel;
	RenderBuffer ssao_noised;
	RenderBuffer ssao_blurred;

	SSRData ssr_data;
	RenderBuffer ssr_trace;
	RenderBuffer ssr_temp;
	RenderBuffer ssr;
	RenderBuffer old_ssr;

	LBuffer lbuffer;
	RenderBuffer composite_temp;
	RenderBuffer composite;
	RenderBuffer old_composite;

	render::Mesh *quad {nullptr};
	ImGuiRenderer *imgui_renderer {nullptr};

	const render::Shader *gbuffer_pass_vertex {nullptr};
	const render::Shader *gbuffer_pass_fragment {nullptr};

	const render::Shader *ssao_pass_vertex {nullptr};
	const render::Shader *ssao_pass_fragment {nullptr};

	const render::Shader *ssao_blur_pass_vertex {nullptr};
	const render::Shader *ssao_blur_pass_fragment {nullptr};

	const render::Shader *ssr_trace_pass_vertex {nullptr};
	const render::Shader *ssr_trace_pass_fragment {nullptr};

	const render::Shader *ssr_resolve_pass_vertex {nullptr};
	const render::Shader *ssr_resolve_pass_fragment {nullptr};

	const render::Shader *temporal_filter_pass_vertex {nullptr};
	const render::Shader *temporal_filter_pass_fragment {nullptr};

	const render::Shader *composite_pass_vertex {nullptr};
	const render::Shader *composite_pass_fragment {nullptr};

	const render::Shader *final_pass_vertex {nullptr};
	const render::Shader *final_pass_fragment {nullptr};
};
