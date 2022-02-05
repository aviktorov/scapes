#pragma once

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
	RenderPassGraphicsBase();
	~RenderPassGraphicsBase() override;

public:
	void init() final;
	void shutdown() final;
	void render(scapes::foundation::render::CommandBuffer command_buffer) final;
	void invalidate() final;

	bool deserialize(const scapes::foundation::serde::yaml::NodeRef node) override;
	bool serialize(scapes::foundation::serde::yaml::NodeRef node) override;

	// TODO: manually set bind set index
	void addInputGroup(const char *name);
	void removeInputGroup(const char *name);
	void removeAllInputGroups();

	// TODO: manually set bind set index and binding index
	void addInputRenderBuffer(const char *name);
	void removeInputRenderBuffer(const char *name);
	void removeAllInputRenderBuffers();

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

	void setSwapChainOutput(
		scapes::foundation::render::RenderPassLoadOp load_op,
		scapes::foundation::render::RenderPassStoreOp store_op,
		scapes::foundation::render::RenderPassClearColor clear_value
	);
	void removeSwapChainOutput();

	SCAPES_INLINE void setRenderGraph(scapes::visual::RenderGraph *graph) { render_graph = graph; }
	SCAPES_INLINE scapes::visual::RenderGraph *getRenderGraph() { return render_graph; }
	SCAPES_INLINE const scapes::visual::RenderGraph *getRenderGraph() const { return render_graph; }

	SCAPES_INLINE void setVertexShader(scapes::visual::ShaderHandle handle) { vertex_shader = handle; }
	SCAPES_INLINE scapes::visual::ShaderHandle getVertexShader() const { return vertex_shader; }

	SCAPES_INLINE void setTessellationControlShader(scapes::visual::ShaderHandle handle) { tessellation_control_shader = handle; }
	SCAPES_INLINE scapes::visual::ShaderHandle getTessellationControlShader() const { return tessellation_control_shader; }

	SCAPES_INLINE void setTesselationEvaluationShader(scapes::visual::ShaderHandle handle) { tessellation_evaluation_shader = handle; }
	SCAPES_INLINE scapes::visual::ShaderHandle getTesselationEvaluationShader() const { return tessellation_evaluation_shader; }

	SCAPES_INLINE void setGeometryShader(scapes::visual::ShaderHandle handle) { geometry_shader = handle; }
	SCAPES_INLINE scapes::visual::ShaderHandle getGeometryShader() const { return geometry_shader; }

	SCAPES_INLINE void setFragmentShader(scapes::visual::ShaderHandle handle) { fragment_shader = handle; }
	SCAPES_INLINE scapes::visual::ShaderHandle getFragmentShader() const { return fragment_shader; }

protected:
	virtual bool canRender() const { return true; }
	virtual void onRender(scapes::foundation::render::CommandBuffer command_buffer) {}
	virtual void onInit() {}
	virtual void onShutdown() {}
	virtual void onInvalidate() {};
	virtual bool onDeserialize(const scapes::foundation::serde::yaml::NodeRef node) { return true; };
	virtual bool onSerialize(scapes::foundation::serde::yaml::NodeRef node) { return true; }

protected:
	struct FrameBufferOutput
	{
		std::string renderbuffer_name;
		scapes::foundation::render::RenderPassLoadOp load_op;
		scapes::foundation::render::RenderPassStoreOp store_op;
		scapes::foundation::render::RenderPassClearValue clear_value;
	};

	struct SwapChainOutput
	{
		scapes::foundation::render::RenderPassLoadOp load_op;
		scapes::foundation::render::RenderPassStoreOp store_op;
		scapes::foundation::render::RenderPassClearColor clear_value;
	};

private:
	void clear();
	void createRenderPassOffscreen();
	void createRenderPassSwapChain();

	void deserializeFrameBufferOutput(scapes::foundation::serde::yaml::NodeRef node, bool is_depthstencil);
	void deserializeSwapChainOutput(scapes::foundation::serde::yaml::NodeRef node);
	void deserializeShader(scapes::foundation::serde::yaml::NodeRef node, scapes::visual::ShaderHandle &handle, scapes::foundation::render::ShaderType shader_type);

	void serializeFrameBufferOutput(scapes::foundation::serde::yaml::NodeRef node, const FrameBufferOutput &data, bool is_depthstencil);
	void serializeSwapChainOutput(scapes::foundation::serde::yaml::NodeRef node, const SwapChainOutput &data);
	void serializeShader(scapes::foundation::serde::yaml::NodeRef node, const char *name, scapes::visual::ShaderHandle handle);

protected:
	std::vector<std::string> input_groups;
	std::vector<std::string> input_render_buffers
;
	std::vector<FrameBufferOutput> color_outputs;
	FrameBufferOutput depthstencil_output;
	bool has_depthstencil_output {false};

	SwapChainOutput swapchain_output;
	bool has_swapchain_output {false};

	scapes::visual::RenderGraph *render_graph {nullptr};
	scapes::foundation::render::Device *device {nullptr};
	scapes::foundation::game::World *world {nullptr};

	scapes::foundation::render::RenderPass render_pass_swapchain {SCAPES_NULL_HANDLE};
	scapes::foundation::render::RenderPass render_pass_offscreen {SCAPES_NULL_HANDLE};
	scapes::foundation::render::GraphicsPipeline graphics_pipeline {SCAPES_NULL_HANDLE};

	scapes::visual::ShaderHandle vertex_shader;
	scapes::visual::ShaderHandle tessellation_control_shader;
	scapes::visual::ShaderHandle tessellation_evaluation_shader;
	scapes::visual::ShaderHandle geometry_shader;
	scapes::visual::ShaderHandle fragment_shader;
};

/*
 */
class RenderPassPrepareOld final : public RenderPassGraphicsBase
{
public:
	static scapes::visual::IRenderPass *create(scapes::visual::RenderGraph *render_graph);

private:
	bool canRender() const final { return first_frame; }
	void onInit() final;
	void onInvalidate() final;
	void onRender(scapes::foundation::render::CommandBuffer command_buffer) final;

private:
	bool first_frame {true};
};

template <>
struct TypeTraits<RenderPassPrepareOld>
{
	static constexpr const char *name = "RenderPassPrepareOld";
};

/*
 */
class RenderPassGeometry final : public RenderPassGraphicsBase
{
public:
	static scapes::visual::IRenderPass *create(scapes::visual::RenderGraph *render_graph);

public:
	SCAPES_INLINE void setMaterialBinding(uint32_t binding) { material_binding = binding; }

private:
	void onInit() final;
	void onRender(scapes::foundation::render::CommandBuffer command_buffer) final;
	bool onDeserialize(const scapes::foundation::serde::yaml::NodeRef node) final;
	bool onSerialize(scapes::foundation::serde::yaml::NodeRef node) final;

private:
	uint32_t material_binding {0};
};

template <>
struct TypeTraits<RenderPassGeometry>
{
	static constexpr const char *name = "RenderPassGeometry";
};

/*
 */
class RenderPassLBuffer final : public RenderPassGraphicsBase
{
public:
	static scapes::visual::IRenderPass *create(scapes::visual::RenderGraph *render_graph);

public:
	SCAPES_INLINE void setLightBinding(uint32_t binding) { light_binding = binding; }

private:
	void onInit() final;
	void onRender(scapes::foundation::render::CommandBuffer command_buffer) final;
	bool onDeserialize(const scapes::foundation::serde::yaml::NodeRef node) final;
	bool onSerialize(scapes::foundation::serde::yaml::NodeRef node) final;

private:
	uint32_t light_binding {0};
};

template <>
struct TypeTraits<RenderPassLBuffer>
{
	static constexpr const char *name = "RenderPassLBuffer";
};

/*
 */
class RenderPassPost final : public RenderPassGraphicsBase
{
public:
	static scapes::visual::IRenderPass *create(scapes::visual::RenderGraph *render_graph);

private:
	void onInit() final;
	void onRender(scapes::foundation::render::CommandBuffer command_buffer) final;
};

template <>
struct TypeTraits<RenderPassPost>
{
	static constexpr const char *name = "RenderPassPost";
};

/*
 */
struct ImGuiContext;
struct ImDrawData;

typedef void * ImTextureID;

class RenderPassImGui final : public RenderPassGraphicsBase
{
public:
	static scapes::visual::IRenderPass *create(scapes::visual::RenderGraph *render_graph);

public:
	ImTextureID fetchTextureID(scapes::foundation::render::Texture texture);
	ImTextureID fetchStorageImageID(scapes::foundation::render::StorageImage image);

	void invalidateTextureIDs();
	void invalidateStorageImageIDs();

	SCAPES_INLINE void setImGuiContext(ImGuiContext *c) { context = c; }
	SCAPES_INLINE const ImGuiContext *getImGuiContext() const { return context; }
	SCAPES_INLINE ImGuiContext *getImGuiContext() { return context; }

private:
	void onInit() final;
	void onShutdown() final;
	void onRender(scapes::foundation::render::CommandBuffer command_buffer) final;

	void updateBuffers(const ImDrawData &draw_data);
	void setupRenderState(const ImDrawData &draw_data);

private:
	ImGuiContext *context {nullptr};

	scapes::foundation::render::Texture font_texture {SCAPES_NULL_HANDLE};
	scapes::foundation::render::VertexBuffer vertices {SCAPES_NULL_HANDLE};
	scapes::foundation::render::IndexBuffer indices {SCAPES_NULL_HANDLE};
	size_t index_buffer_size {0};
	size_t vertex_buffer_size {0};

	scapes::foundation::render::BindSet font_bind_set {SCAPES_NULL_HANDLE};
	std::map<scapes::foundation::render::Texture, scapes::foundation::render::BindSet> registered_textures;
	std::map<scapes::foundation::render::StorageImage, scapes::foundation::render::BindSet> registered_images;
};

template <>
struct TypeTraits<RenderPassImGui>
{
	static constexpr const char *name = "RenderPassImGui";
};

/*
 */
class RenderPassSwapRenderBuffers : public scapes::visual::IRenderPass
{
public:
	static scapes::visual::IRenderPass *create(scapes::visual::RenderGraph *render_graph);

public:
	RenderPassSwapRenderBuffers();
	~RenderPassSwapRenderBuffers() override;

public:
	void init() final;
	void shutdown() final;
	void render(scapes::foundation::render::CommandBuffer command_buffer) final;
	void invalidate() final;
	void clear();

	bool deserialize(const scapes::foundation::serde::yaml::NodeRef node) override;
	bool serialize(scapes::foundation::serde::yaml::NodeRef node) override;

	void addSwapPair(const char *src, const char *dst);
	void removeAllSwapPairs();

	SCAPES_INLINE void setRenderGraph(scapes::visual::RenderGraph *graph) { render_graph = graph; }
	SCAPES_INLINE scapes::visual::RenderGraph *getRenderGraph() { return render_graph; }
	SCAPES_INLINE const scapes::visual::RenderGraph *getRenderGraph() const { return render_graph; }

private:
	struct SwapPair
	{
		std::string src;
		std::string dst;
	};

	scapes::visual::RenderGraph *render_graph {nullptr};
	std::vector<SwapPair> pairs;
};

template <>
struct TypeTraits<RenderPassSwapRenderBuffers>
{
	static constexpr const char *name = "RenderPassSwapRenderBuffers";
};
