#pragma once

#include <render/shaders/Compiler.h>
#include <render/backend/Driver.h>
#include <common/ResourceManager.h>
#include <map>

struct RenderFrame;
class SwapChain;
struct Texture;
struct Shader;

struct ImGuiContext;
struct ImDrawData;

typedef void* ImTextureID;

/*
 */
class ImGuiRenderer
{
public:
	ImGuiRenderer(render::backend::Driver *driver, render::shaders::Compiler *compiler, resources::ResourceManager *resource_manager);
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
	resources::ResourceManager *resource_manager {nullptr};

	resources::ResourceHandle<Texture> font_texture;
	resources::ResourceHandle<Shader> vertex_shader;
	resources::ResourceHandle<Shader> fragment_shader;

	render::backend::PipelineState *pipeline_state {nullptr};
	render::backend::VertexBuffer *vertices {nullptr};
	render::backend::IndexBuffer *indices {nullptr};
	size_t index_buffer_size {0};
	size_t vertex_buffer_size {0};

	render::backend::BindSet *font_bind_set {nullptr};
	std::map<const render::backend::Texture *, render::backend::BindSet *> registered_textures;
};
