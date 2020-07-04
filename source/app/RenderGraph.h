#pragma once

#include <render/backend/driver.h>

class RenderScene;
class Scene;
class VulkanShader;
struct VulkanRenderFrame;

struct GBuffer
{
	render::backend::Texture *base_color {nullptr};     // rgba8, rga - albedo for dielectrics, f0 for conductors, a - unused
	render::backend::Texture *depth {nullptr};          // r32f,  linear depth
	render::backend::Texture *shading {nullptr};        // rg8,   r - roughness, g - metalness
	render::backend::Texture *normal {nullptr};         // rg16f, packed normal without z
	render::backend::FrameBuffer *framebuffer {nullptr};
};

struct LBuffer
{
	render::backend::Texture *diffuse {nullptr};        // rgb16f, hdr linear color
	render::backend::Texture *specular {nullptr};       // rga16f, hdr linear color
	render::backend::FrameBuffer *framebuffer {nullptr};
};

/*
 */
class RenderGraph
{
public:
	RenderGraph(render::backend::Driver *driver)
		: driver(driver) { }

	virtual ~RenderGraph();

	void init(const RenderScene *scene, uint32_t width, uint32_t height);
	void shutdown();
	void resize(uint32_t width, uint32_t height);
	void render(const Scene *scene, const VulkanRenderFrame &frame);

	const GBuffer &getGBuffer() const { return gbuffer; }
	const LBuffer &getLBuffer() const { return lbuffer; }

private:
	void initGBuffer(uint32_t width, uint32_t height);
	void shutdownGBuffer();

	void initLBuffer(uint32_t width, uint32_t height);
	void shutdownLBuffer();

	void renderGBuffer(const Scene *scene, const VulkanRenderFrame &frame);

private:
	render::backend::Driver *driver {nullptr};

	GBuffer gbuffer;
	LBuffer lbuffer;

	const VulkanShader *gbuffer_pass_vertex {nullptr};
	const VulkanShader *gbuffer_pass_fragment {nullptr};
};
