#pragma once

#include <scapes/foundation/json/Json.h>
#include <scapes/foundation/math/Math.h>
#include <scapes/foundation/render/Device.h>

#include <scapes/visual/Resources.h>
#include <scapes/visual/RenderGraph.h>

#include <string>
#include <vector>
#include <map>

/*
 */
class RenderPassGraphicsBase : public scapes::visual::IRenderPass
{
public:
	~RenderPassGraphicsBase() override;

public:
	void init(const scapes::visual::RenderGraph *render_graph) override;
	void shutdown() override;
	void invalidate() override;
	void invalidateBindings();

	bool deserialize(const scapes::foundation::json::Document &document) override { return false; }
	scapes::foundation::json::Document serialize() override { return scapes::foundation::json::Document(); }

	// TODO: manually set bind set index
	void addInputParameterGroup(const char *name);
	void removeInputParameterGroup(const char *name);
	void removeAllInputParameterGroups();

	// TODO: manually set bind set index and binding index
	void addInputTexture(const char *name);
	void removeInputTexture(const char *name);
	void removeAllInputTextures();

	void addColorOutput(
		const char *name,
		scapes::foundation::render::RenderPassLoadOp load_op,
		scapes::foundation::render::RenderPassStoreOp store_op,
		scapes::foundation::render::RenderPassClearColor clear_value
	);

	void removeColorOutput(const char *name);
	void removeAllColorOutputs();

	void setDepthStencilOutput(
		const char *name,
		scapes::foundation::render::RenderPassLoadOp load_op,
		scapes::foundation::render::RenderPassStoreOp store_op,
		scapes::foundation::render::RenderPassClearDepthStencil clear_value
	);
	void removeDepthStencilOutput();

private:
	void clear();
	void createRenderPassOffscreen();
	void createRenderPassSwapChain();
	void createFrameBuffer();

protected:
	struct Output
	{
		std::string texture_name;
		scapes::foundation::render::RenderPassLoadOp load_op;
		scapes::foundation::render::RenderPassStoreOp store_op;
		scapes::foundation::render::RenderPassClearValue clear_value;
	};

	std::vector<std::string> input_groups;
	std::vector<std::string> input_textures;
	std::vector<Output> color_outputs;
	Output depthstencil_output;
	bool has_depthstencil_output {false};

	const scapes::visual::RenderGraph *render_graph {nullptr};
	scapes::foundation::render::Device *device {nullptr};
	scapes::foundation::game::World *world {nullptr};

	scapes::foundation::render::BindSet *texture_bindings {nullptr};
	scapes::foundation::render::RenderPass *render_pass {nullptr}; // naive impl
	scapes::foundation::render::FrameBuffer *frame_buffer {nullptr}; // naive impl
	scapes::foundation::render::SwapChain *swap_chain {nullptr};
	scapes::foundation::render::PipelineState *pipeline_state {nullptr};
};

/*
 */
class RenderPassPrepareOld : public RenderPassGraphicsBase
{
public:
	void init(const scapes::visual::RenderGraph *render_graph) final;
	void invalidate() final;
	void render(scapes::foundation::render::CommandBuffer *command_buffer) final;

	SCAPES_INLINE void setFullscreenQuad(
		scapes::visual::ShaderHandle vertex_shader,
		scapes::visual::MeshHandle mesh
	)
	{
		fullscreen_quad_vertex_shader = vertex_shader;
		fullscreen_quad_mesh = mesh;
	}

private:
	bool first_frame {true};

	scapes::visual::ShaderHandle fullscreen_quad_vertex_shader;
	scapes::visual::MeshHandle fullscreen_quad_mesh;
};

/*
 */
class RenderPassGeometry : public RenderPassGraphicsBase
{
public:
	void init(const scapes::visual::RenderGraph *render_graph) final;
	void render(scapes::foundation::render::CommandBuffer *command_buffer) final;

	SCAPES_INLINE void setFragmentShader(scapes::visual::ShaderHandle handle) { fragment_shader = handle; }
	SCAPES_INLINE void setVertexShader(scapes::visual::ShaderHandle handle) { vertex_shader = handle; }
	SCAPES_INLINE void setMaterialBinding(uint32_t binding) { material_binding = binding; }

private:
	uint32_t material_binding {0};
	scapes::visual::ShaderHandle vertex_shader;
	scapes::visual::ShaderHandle fragment_shader;
};

/*
 */
class RenderPassLBuffer : public RenderPassGraphicsBase
{
public:
	void init(const scapes::visual::RenderGraph *render_graph) final;
	void render(scapes::foundation::render::CommandBuffer *command_buffer) final;

	SCAPES_INLINE void setLightBinding(uint32_t binding) { light_binding = binding; }

private:
	uint32_t light_binding {0};
};

/*
 */
class RenderPassPost final : public RenderPassGraphicsBase
{
public:
	void init(const scapes::visual::RenderGraph *render_graph) final;
	void render(scapes::foundation::render::CommandBuffer *command_buffer) final;

	SCAPES_INLINE void setFullscreenQuad(
		scapes::visual::ShaderHandle vertex_shader,
		scapes::visual::MeshHandle mesh
	)
	{
		fullscreen_quad_vertex_shader = vertex_shader;
		fullscreen_quad_mesh = mesh;
	}

	SCAPES_INLINE void setFragmentShader(scapes::visual::ShaderHandle handle) { fragment_shader = handle; }

private:
	scapes::visual::ShaderHandle fullscreen_quad_vertex_shader;
	scapes::visual::MeshHandle fullscreen_quad_mesh;
	scapes::visual::ShaderHandle fragment_shader;
};

/*
 */
struct ImGuiContext;
struct ImDrawData;

typedef void * ImTextureID;

class RenderPassImGui final : public RenderPassGraphicsBase
{
public:
	RenderPassImGui(ImGuiContext *context);
	virtual ~RenderPassImGui();

	void init(const scapes::visual::RenderGraph *render_graph) final;
	void shutdown() final;
	void invalidate() final;
	void render(scapes::foundation::render::CommandBuffer *command_buffer) final;

	ImTextureID fetchTextureID(const scapes::foundation::render::Texture *texture);
	void invalidateTextureIDs();

	SCAPES_INLINE void setVertexShader(scapes::visual::ShaderHandle shader) { vertex_shader = shader; }
	SCAPES_INLINE void setFragmentShader(scapes::visual::ShaderHandle shader) { fragment_shader = shader; }

private:
	void updateBuffers(const ImDrawData *draw_data);
	void setupRenderState(const ImDrawData *draw_data);

private:
	ImGuiContext *context {nullptr};

	scapes::visual::ShaderHandle vertex_shader;
	scapes::visual::ShaderHandle fragment_shader;

	scapes::foundation::render::Texture *font_texture {nullptr};
	scapes::foundation::render::VertexBuffer *vertices {nullptr};
	scapes::foundation::render::IndexBuffer *indices {nullptr};
	size_t index_buffer_size {0};
	size_t vertex_buffer_size {0};

	scapes::foundation::render::BindSet *font_bind_set {nullptr};
	std::map<const scapes::foundation::render::Texture *, scapes::foundation::render::BindSet *> registered_textures;
};
