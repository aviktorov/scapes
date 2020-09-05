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
	struct RenderFrame;
}

struct GBuffer
{
	render::backend::Texture *base_color {nullptr};     // rgba8, rga - albedo for dielectrics, f0 for conductors, a - unused
	render::backend::Texture *shading {nullptr};        // rg8,   r - roughness, g - metalness
	render::backend::Texture *normal {nullptr};         // rg16f, packed normal without z
	render::backend::Texture *depth {nullptr};          // d32f,  linear depth
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

struct SSAO
{
	struct CPUData
	{
		enum
		{
			MAX_SAMPLES = 256,
		};

		uint32_t num_samples {32};
		float radius {1.0f};
		float intensity {1.0f};
		float alignment[1]; // we need 16 byte alignment
		glm::vec4 samples[MAX_SAMPLES];
	};

	CPUData *cpu_data {nullptr};
	render::backend::UniformBuffer *gpu_data {nullptr};
	render::backend::BindSet *internal_bindings {nullptr};

	render::backend::Texture *texture {nullptr};
	render::backend::FrameBuffer *framebuffer {nullptr};
	render::backend::BindSet *bindings {nullptr};

};

// TODO: SSR

struct Composite
{
	render::backend::Texture *hdr_color {nullptr};       // rga16f, hdr linear color
	render::backend::FrameBuffer *framebuffer {nullptr};
	render::backend::BindSet *bindings {nullptr};
};

/*
 */
class RenderGraph
{
public:
	RenderGraph(render::backend::Driver *driver)
		: driver(driver) { }

	virtual ~RenderGraph();

	void init(const ApplicationResources *resources, uint32_t width, uint32_t height);
	void shutdown();
	void resize(uint32_t width, uint32_t height);
	void render(const Scene *scene, const render::RenderFrame &frame);

	ImTextureID fetchTextureID(const render::backend::Texture *texture);

	const GBuffer &getGBuffer() const { return gbuffer; }
	const LBuffer &getLBuffer() const { return lbuffer; }
	const SSAO &getSSAO() const { return ssao; }
	SSAO &getSSAO() { return ssao; }

private:
	void initGBuffer(uint32_t width, uint32_t height);
	void shutdownGBuffer();

	void initSSAO(uint32_t width, uint32_t height);
	void shutdownSSAO();

	void initLBuffer(uint32_t width, uint32_t height);
	void shutdownLBuffer();

	void initComposite(uint32_t width, uint32_t height);
	void shutdownComposite();

	void renderGBuffer(const Scene *scene, const render::RenderFrame &frame);
	void renderSSAO(const Scene *scene, const render::RenderFrame &frame);
	void renderLBuffer(const Scene *scene, const render::RenderFrame &frame);
	void renderComposite(const Scene *scene, const render::RenderFrame &frame);
	void renderFinal(const Scene *scene, const render::RenderFrame &frame);

private:
	render::backend::Driver *driver {nullptr};

	GBuffer gbuffer;
	SSAO ssao;
	LBuffer lbuffer;
	Composite composite;

	render::Mesh *quad {nullptr};
	ImGuiRenderer *imgui_renderer {nullptr};

	const render::Shader *gbuffer_pass_vertex {nullptr};
	const render::Shader *gbuffer_pass_fragment {nullptr};

	const render::Shader *ssao_pass_vertex {nullptr};
	const render::Shader *ssao_pass_fragment {nullptr};

	const render::Shader *composite_pass_vertex {nullptr};
	const render::Shader *composite_pass_fragment {nullptr};

	const render::Shader *final_pass_vertex {nullptr};
	const render::Shader *final_pass_fragment {nullptr};
};
