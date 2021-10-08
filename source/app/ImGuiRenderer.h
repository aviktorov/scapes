#pragma once

#include <render/shaders/Compiler.h>
#include <render/backend/Driver.h>
#include <map>

struct RenderFrame;
class SwapChain;
struct Texture;
class Shader;

struct ImGuiContext;
struct ImDrawData;

typedef void* ImTextureID;

/*
 */
class ImGuiRenderer
{
public:
	ImGuiRenderer(render::backend::Driver *driver, render::shaders::Compiler *compiler);
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
	render::shaders::Compiler *compiler {nullptr};
	render::backend::Texture *font_texture {nullptr};
	render::backend::PipelineState *pipeline_state {nullptr};
	Shader *vertex_shader {nullptr};
	Shader *fragment_shader {nullptr};

	render::backend::VertexBuffer *vertices {nullptr};
	render::backend::IndexBuffer *indices {nullptr};
	size_t index_buffer_size {0};
	size_t vertex_buffer_size {0};
	bool flipped {true};

	render::backend::BindSet *font_bind_set {nullptr};
	std::map<const render::backend::Texture *, render::backend::BindSet *> registered_textures;
};
