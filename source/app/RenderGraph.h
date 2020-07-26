#pragma once

#include <render/backend/driver.h>

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

// TODO: SSAO
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

private:
	void initGBuffer(uint32_t width, uint32_t height);
	void shutdownGBuffer();

	void initLBuffer(uint32_t width, uint32_t height);
	void shutdownLBuffer();

	void initComposite(uint32_t width, uint32_t height);
	void shutdownComposite();

	void renderGBuffer(const Scene *scene, const render::RenderFrame &frame);
	void renderLBuffer(const Scene *scene, const render::RenderFrame &frame);
	void renderComposite(const Scene *scene, const render::RenderFrame &frame);
	void renderFinal(const Scene *scene, const render::RenderFrame &frame);

private:
	render::backend::Driver *driver {nullptr};

	GBuffer gbuffer;
	LBuffer lbuffer;
	Composite composite;

	render::Mesh *quad {nullptr};
	ImGuiRenderer *imgui_renderer {nullptr};

	const render::Shader *gbuffer_pass_vertex {nullptr};
	const render::Shader *gbuffer_pass_fragment {nullptr};

	const render::Shader *composite_pass_vertex {nullptr};
	const render::Shader *composite_pass_fragment {nullptr};

	const render::Shader *final_pass_vertex {nullptr};
	const render::Shader *final_pass_fragment {nullptr};
};
