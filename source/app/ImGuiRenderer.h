#pragma once

#include <scapes/foundation/resources/ResourceManager.h>
#include <scapes/visual/Fwd.h>

#include <map>

class SwapChain;

struct ImGuiContext;
struct ImDrawData;

typedef void* ImTextureID;

/*
 */
class ImGuiRenderer
{
public:
	ImGuiRenderer(scapes::foundation::render::Device *device, scapes::visual::API *visual_api);
	virtual ~ImGuiRenderer();

	void init(ImGuiContext *imguiContext);
	void shutdown();
	void render(scapes::foundation::render::CommandBuffer *command_buffer);

	ImTextureID fetchTextureID(const scapes::foundation::render::Texture *texture);
	void invalidateTextureIDs();

private:
	void updateBuffers(const ImDrawData *draw_data);
	void setupRenderState(const ImDrawData *draw_data);

private:
	scapes::foundation::render::Device *device {nullptr};
	scapes::visual::API *visual_api {nullptr};

	scapes::visual::TextureHandle font_texture;
	scapes::visual::ShaderHandle vertex_shader;
	scapes::visual::ShaderHandle fragment_shader;

	scapes::foundation::render::PipelineState *pipeline_state {nullptr};
	scapes::foundation::render::VertexBuffer *vertices {nullptr};
	scapes::foundation::render::IndexBuffer *indices {nullptr};
	size_t index_buffer_size {0};
	size_t vertex_buffer_size {0};

	scapes::foundation::render::BindSet *font_bind_set {nullptr};
	std::map<const scapes::foundation::render::Texture *, scapes::foundation::render::BindSet *> registered_textures;
};
