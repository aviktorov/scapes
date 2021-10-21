#pragma once

#include <scapes/visual/Fwd.h>

#include <render/backend/Driver.h>
#include <map>

struct RenderFrame;
class SwapChain;

struct ImGuiContext;
struct ImDrawData;

typedef void* ImTextureID;

/*
 */
class ImGuiRenderer
{
public:
	ImGuiRenderer(render::backend::Driver *driver, scapes::visual::API *visual_api);
	virtual ~ImGuiRenderer();

	void init(ImGuiContext *imguiContext);
	void shutdown();
	void render(render::backend::CommandBuffer *command_buffer);

	ImTextureID fetchTextureID(const render::backend::Texture *texture);
	void invalidateTextureIDs();

private:
	void updateBuffers(const ImDrawData *draw_data);
	void setupRenderState(const ImDrawData *draw_data);

private:
	render::backend::Driver *driver {nullptr};
	scapes::visual::API *visual_api {nullptr};

	scapes::visual::TextureHandle font_texture;
	scapes::visual::ShaderHandle vertex_shader;
	scapes::visual::ShaderHandle fragment_shader;

	render::backend::PipelineState *pipeline_state {nullptr};
	render::backend::VertexBuffer *vertices {nullptr};
	render::backend::IndexBuffer *indices {nullptr};
	size_t index_buffer_size {0};
	size_t vertex_buffer_size {0};

	render::backend::BindSet *font_bind_set {nullptr};
	std::map<const render::backend::Texture *, render::backend::BindSet *> registered_textures;
};
