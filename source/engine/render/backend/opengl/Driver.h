#pragma once

#include <render/backend/Driver.h>
#include <render/backend/opengl/Platform.h>

#include <glad.h>

namespace render::backend::opengl
{

struct Buffer
{
	GLuint id {0};
	GLenum type {0};
	GLsizeiptr size {0};
	GLbitfield immutable_usage_flags {0};
	GLboolean immutable {GL_FALSE};
	GLenum mutable_usage_hint {GL_STATIC_DRAW};
};

struct VertexBuffer : public backend::VertexBuffer
{
	GLuint vao_id {0};
	GLuint num_vertices {0};
	GLuint vertex_size {0};
	Buffer *data {nullptr};
};

struct IndexBuffer : public backend::IndexBuffer
{
	GLuint num_indices {0};
	GLenum index_format {GL_UNSIGNED_INT};
	Buffer *data {nullptr};
};

struct Texture : public backend::Texture
{
	GLuint id {0};
	GLenum type {GL_TEXTURE_2D};
	GLenum internal_format {GL_RGBA8};
	GLsizei width {0};
	GLsizei height {0};
	GLsizei depth {0};
	GLsizei layers {1};
	GLint mips {1};
	GLsizei num_samples {1};
};

struct FrameBufferColorAttachment
{
	GLuint id {0};
	GLenum texture_type {GL_NONE};
	GLenum type {GL_NONE};
	GLint base_mip {0};
	GLint base_layer {0};
	GLuint num_layers {1};
};

struct FrameBufferDepthStencilAttachment
{
	GLuint id {0};
	GLenum type {GL_NONE};
};

struct FrameBuffer : public backend::FrameBuffer
{
	enum
	{
		MAX_COLOR_ATTACHMENTS = 16,
	};

	uint32_t width {0};
	uint32_t height {0};

	GLuint main_fbo_id {0};
	uint8_t num_color_attachments {0};
	FrameBufferColorAttachment color_attachments[FrameBuffer::MAX_COLOR_ATTACHMENTS];
	uint32_t color_attachment_indices[FrameBuffer::MAX_COLOR_ATTACHMENTS];

	FrameBufferDepthStencilAttachment depth_stencil_attachment;
	uint32_t depth_stencil_attachment_index {0};

	GLuint resolve_fbo_id {0};
	uint8_t num_resolve_color_attachments {0};
	FrameBufferColorAttachment resolve_color_attachments[FrameBuffer::MAX_COLOR_ATTACHMENTS];
	uint32_t resolve_color_attachment_indices[FrameBuffer::MAX_COLOR_ATTACHMENTS];
};

enum class CommandType : uint8_t
{
	NONE,
	SET_VIEWPORT,
	SET_SCISSOR,
	SET_DEPTH_STENCIL_STATE,
	SET_BLEND_STATE,
	SET_RASTERIZER_STATE,
	CLEAR_COLOR_BUFFER,
	CLEAR_DEPTHSTENCIL_BUFFER,
	BLIT_FRAME_BUFFER,
	BIND_FRAME_BUFFER,
	BIND_UNIFORM_BUFFER,
	BIND_TEXTURE,
	BIND_SHADER,
	BIND_VERTEX_BUFFER,
	BIND_INDEX_BUFFER,
	DRAW_INDEXED_PRIMITIVE,
	MAX,
};

struct Command
{
	struct Rect
	{
		GLint x {0};
		GLint y {0};
		GLsizei width {0};
		GLsizei height {0};
	};

	struct DepthStencilState
	{
		uint8_t depth_test : 1;
		uint8_t depth_write : 1;
		GLenum depth_compare_func {GL_LEQUAL};

		// TODO: stencil state
	};

	struct BlendState
	{
		uint8_t enabled : 1;
		GLenum src_factor {GL_ZERO};
		GLenum dst_factor {GL_ZERO};
	};

	struct RasterizerState
	{
		GLenum cull_mode {GL_NONE};
	};

	struct ClearColorBuffer
	{
		GLint buffer {0};
		GLfloat clear_color[4] {0.0f, 0.0f, 0.0f, 0.0f};
	};

	struct ClearDepthStencilBuffer
	{
		GLfloat depth {0.0f};
		GLint stencil {0};
	};

	struct BlitFrameBuffer
	{
		GLuint dst_fbo_id {0};
		GLuint src_fbo_id {0};
		GLint width {0};
		GLint height {0};
		GLbitfield mask {0};
		GLsizei num_draw_buffers {0};
		GLenum draw_buffers[FrameBuffer::MAX_COLOR_ATTACHMENTS];
	};

	struct BindFrameBuffer
	{
		GLuint id {0};
		GLenum target {GL_FRAMEBUFFER};
	};

	struct BindUniformBuffer
	{
		GLuint binding {0};
		GLuint id {0};
		GLintptr offset {0};
		GLsizeiptr size {0};
	};

	struct BindTexture
	{
		GLuint binding {0};
		GLenum type {GL_TEXTURE_2D};
		GLuint id {0};
	};

	struct BindShader
	{
		GLuint pipeline_id {0};
		GLuint shader_id {0};
		GLenum shader_stages {0};
	};

	struct BindVertexBuffer
	{
		GLuint vao_id {0};
	};

	struct BindIndexBuffer
	{
		GLuint id {0};
	};

	struct DrawIndexedPrimitive
	{
		GLenum primitive_type {GL_TRIANGLES};
		GLenum index_format {0};
		GLuint num_indices {0};
		GLuint base_index {0};
		GLint base_vertex {0};
		GLsizei num_instances {1};
	};

	CommandType type {CommandType::NONE};
	Command *next {nullptr};

	union
	{
		Rect viewport;
		Rect scissor;
		DepthStencilState depth_stencil_state;
		BlendState blend_state;
		RasterizerState rasterizer_state;
		ClearColorBuffer clear_color_buffer;
		ClearDepthStencilBuffer clear_depth_stencil_buffer;
		BlitFrameBuffer blit_frame_buffer;
		BindFrameBuffer bind_frame_buffer;
		BindUniformBuffer bind_uniform_buffer;
		BindTexture bind_texture;
		BindShader bind_shader;
		BindVertexBuffer bind_vertex_buffer;
		BindIndexBuffer bind_index_buffer;
		DrawIndexedPrimitive draw_indexed_primitive;
	};

	Command() { }
};

enum class CommandBufferState : uint8_t
{
	INITIAL = 0,
	RECORDING,
	EXECUTABLE,
	PENDING,
	INVALID,
};

struct CommandBuffer : public render::backend::CommandBuffer
{
	CommandBufferType type {CommandBufferType::PRIMARY};
	CommandBufferState state {CommandBufferState::INITIAL};

	Buffer *push_constants {nullptr};
	void *push_constants_mapped_memory {nullptr};

	// TODO: sync primitives

	Command *first {nullptr};
	Command *last {nullptr};
};

struct UniformBuffer : public backend::UniformBuffer
{
	Buffer *data {nullptr};
};

struct Shader : public backend::Shader
{
	GLuint id {0};
	GLint compiled {GL_FALSE};
	GLenum type {GL_FRAGMENT_SHADER};
};

struct BindSet : public backend::BindSet
{
	enum
	{
		MAX_BINDINGS = 16,
	};

	enum class DataType
	{
		TEXTURE = 0,
		UNIFORM_BUFFER,
	};

	union Data
	{
		struct Texture
		{
			GLuint id;
			GLenum type;
			GLint base_mip;
			GLuint num_mips;
			GLint base_layer;
			GLuint num_layers;
		} texture;
		struct UBO
		{
			GLuint id;
			uint32_t offset;
			GLsizeiptr size;
		} ubo;
	};

	Data datas[MAX_BINDINGS];
	DataType types[MAX_BINDINGS];
	uint32_t binding_used {0};
	uint32_t binding_dirty {0};
};

struct SwapChain : public backend::SwapChain
{
	Surface *surface {nullptr};
	uint32_t num_images {0};
	uint32_t current_image {0};

	uint32_t width {0};
	uint32_t height {0};

	GLuint msaa_fbo_id {0};
	GLuint msaa_color_id {0};
	GLuint msaa_depth_stencil_id {0};

	Format color_format {Format::R8G8B8A8_UNORM};
	Format depth_stencil_format {Format::D24_UNORM_S8_UINT};
};

class Driver : public backend::Driver
{
public:
	Driver(const char *application_name, const char *engine_name);
	virtual ~Driver();

public:
	backend::VertexBuffer *createVertexBuffer(
		BufferType type,
		uint16_t vertex_size,
		uint32_t num_vertices,
		uint8_t num_attributes,
		const backend::VertexAttribute *attributes,
		const void *data
	) final;

	backend::IndexBuffer *createIndexBuffer(
		BufferType type,
		IndexFormat index_format,
		uint32_t num_indices,
		const void *data
	) final;

	backend::Texture *createTexture2D(
		uint32_t width,
		uint32_t height,
		uint32_t num_mipmaps,
		Format format,
		const void *data = nullptr,
		uint32_t num_data_mipmaps = 1
	) final;

	backend::Texture *createTexture2DMultisample(
		uint32_t width,
		uint32_t height,
		Format format,
		Multisample samples
	) final;

	backend::Texture *createTexture2DArray(
		uint32_t width,
		uint32_t height,
		uint32_t num_mipmaps,
		uint32_t num_layers,
		Format format,
		const void *data = nullptr,
		uint32_t num_data_mipmaps = 1,
		uint32_t num_data_layers = 1
	) final;

	backend::Texture *createTexture3D(
		uint32_t width,
		uint32_t height,
		uint32_t depth,
		uint32_t num_mipmaps,
		Format format,
		const void *data = nullptr,
		uint32_t num_data_mipmaps = 1
	) final;

	backend::Texture *createTextureCube(
		uint32_t size,
		uint32_t num_mipmaps,
		Format format,
		const void *data = nullptr,
		uint32_t num_data_mipmaps = 1
	) final;

	backend::FrameBuffer *createFrameBuffer(
		uint8_t num_color_attachments,
		const FrameBufferAttachment *color_attachments,
		const FrameBufferAttachment *depthstencil_attachment
	) final;

	backend::CommandBuffer *createCommandBuffer(
		CommandBufferType type
	) final;

	backend::UniformBuffer *createUniformBuffer(
		BufferType type,
		uint32_t size,
		const void *data = nullptr
	) final;

	backend::Shader *createShaderFromSource(
		ShaderType type,
		uint32_t size,
		const char *data,
		const char *path = nullptr
	) final;

	backend::Shader *createShaderFromIL(
		const shaders::ShaderIL *shader_il
	) final;

	backend::BindSet *createBindSet(
	) final;

	backend::SwapChain *createSwapChain(
		void *native_window,
		uint32_t width,
		uint32_t height,
		Multisample samples
	) final;

	void destroyVertexBuffer(backend::VertexBuffer *vertex_buffer) final;
	void destroyIndexBuffer(backend::IndexBuffer *index_buffer) final;
	void destroyTexture(backend::Texture *texture) final;
	void destroyFrameBuffer(backend::FrameBuffer *frame_buffer) final;
	void destroyCommandBuffer(backend::CommandBuffer *command_buffer) final;
	void destroyUniformBuffer(backend::UniformBuffer *uniform_buffer) final;
	void destroyShader(backend::Shader *shader) final;
	void destroyBindSet(backend::BindSet *bind_set) final;
	void destroySwapChain(backend::SwapChain *swap_chain) final;

public:
	Multisample getMaxSampleCount() final;

	uint32_t getNumSwapChainImages(const backend::SwapChain *swap_chain) final;

	void setTextureSamplerWrapMode(backend::Texture *texture, SamplerWrapMode mode) final;
	void setTextureSamplerDepthCompare(backend::Texture *texture, bool enabled, DepthCompareFunc func) final;
	void generateTexture2DMipmaps(backend::Texture *texture) final;

public:
	void *map(backend::VertexBuffer *vertex_buffer) final;
	void unmap(backend::VertexBuffer *vertex_buffer) final;

	void *map(backend::IndexBuffer *index_buffer) final;
	void unmap(backend::IndexBuffer *index_buffer) final;

	void *map(backend::UniformBuffer *uniform_buffer) final;
	void unmap(backend::UniformBuffer *uniform_buffer) final;

public:
	bool acquire(
		backend::SwapChain *swap_chain,
		uint32_t *new_image
	) final;

	bool present(
		backend::SwapChain *swap_chain,
		uint32_t num_wait_command_buffers,
		backend::CommandBuffer * const *wait_command_buffers
	) final;

	bool wait(
		uint32_t num_wait_command_buffers,
		backend::CommandBuffer * const *wait_command_buffers
	) final;

	void wait() final;

public:
	// bindings
	void bindUniformBuffer(
		backend::BindSet *bind_set,
		uint32_t binding,
		const backend::UniformBuffer *uniform_buffer
	) final;

	void bindTexture(
		backend::BindSet *bind_set,
		uint32_t binding,
		const backend::Texture *texture
	) final;

	void bindTexture(
		backend::BindSet *bind_set,
		uint32_t binding,
		const backend::Texture *texture,
		uint32_t base_mip,
		uint32_t num_mips,
		uint32_t base_layer,
		uint32_t num_layers
	) final;

public:
	// pipeline state
	void clearPushConstants(
	) final;

	void setPushConstants(
		uint8_t size,
		const void *data
	) final;

	void clearBindSets(
	) final;

	void allocateBindSets(
		uint8_t size
	) final;

	void pushBindSet(
		backend::BindSet *bind_set
	) final;

	void setBindSet(
		uint32_t binding,
		backend::BindSet *bind_set
	) final;

	void clearShaders(
	) final;

	void setShader(
		ShaderType type,
		const backend::Shader *shader
	) final;

public:
	// render state
	void setViewport(
		float x,
		float y,
		float width,
		float height
	) final;

	void setScissor(
		int32_t x,
		int32_t y,
		uint32_t width,
		uint32_t height
	) final;

	void setCullMode(
		CullMode mode
	) final;

	void setDepthTest(
		bool enabled
	) final;

	void setDepthWrite(
		bool enabled
	) final;

	void setDepthCompareFunc(
		DepthCompareFunc func
	) final;

	void setBlending(
		bool enabled
	) final;

	void setBlendFactors(
		BlendFactor src_factor,
		BlendFactor dest_factor
	) final;

public:
	// command buffers
	bool resetCommandBuffer(
		backend::CommandBuffer *command_buffer
	) final;

	bool beginCommandBuffer(
		backend::CommandBuffer *command_buffer
	) final;

	bool endCommandBuffer(
		backend::CommandBuffer *command_buffer
	) final;

	bool submit(
		backend::CommandBuffer *command_buffer
	) final;

	bool submitSyncked(
		backend::CommandBuffer *command_buffer,
		const backend::SwapChain *wait_swap_chain
	) final;

	bool submitSyncked(
		backend::CommandBuffer *command_buffer,
		uint32_t num_wait_command_buffers,
		backend::CommandBuffer * const *wait_command_buffers
	) final;

public:
	// render commands
	void beginRenderPass(
		backend::CommandBuffer *command_buffer,
		const backend::FrameBuffer *frame_buffer,
		const RenderPassInfo *info
	) final;

	void beginRenderPass(
		backend::CommandBuffer *command_buffer,
		const backend::SwapChain *swap_chain,
		const RenderPassInfo *info
	) final;

	void endRenderPass(
		backend::CommandBuffer *command_buffer
	) final;

	void drawIndexedPrimitive(
		backend::CommandBuffer *command_buffer,
		const RenderPrimitive *render_primitive
	) final;

protected:
	bool init();
	void shutdown();

private:
	enum
	{
		MAX_BIND_SETS = 16,
		MAX_SHADERS = static_cast<int>(ShaderType::MAX),
		MAX_PUSH_CONSTANT_SIZE = 128,
		UNIFORM_BUFFER_BINDING_OFFSET = 1, // zero slot is taken by push constants
	};

	struct PipelineState
	{
		GLint viewport_x {0};
		GLint viewport_y {0};
		GLsizei viewport_width {0};
		GLsizei viewport_height {0};
		GLint scissor_x {0};
		GLint scissor_y {0};
		GLsizei scissor_width {0};
		GLsizei scissor_height {0};
		uint8_t depth_test : 1;
		uint8_t depth_write : 1;
		GLenum depth_compare_func {GL_LEQUAL};
		uint8_t blend : 1;
		GLenum blend_src_factor {GL_ZERO};
		GLenum blend_dst_factor {GL_ZERO};
		GLenum cull_mode {GL_NONE};
		uint8_t num_bind_sets {0};
		BindSet *bound_bind_sets[MAX_BIND_SETS];
		GLuint bound_shaders[MAX_SHADERS];
	};

	struct PipelineStateOverrides
	{
		uint8_t viewport : 1;
		uint8_t scissor : 1;
		uint8_t depth_stencil_state : 1;
		uint8_t blend_state : 1;
		uint8_t rasterizer_state : 1;
		uint16_t bound_bind_sets;
		uint16_t bound_shaders;
	};

	struct RenderPassState
	{
		bool resolve {false};
		GLuint src_fbo_id = 0;
		GLuint dst_fbo_id = 0;
		GLint width = 0;
		GLint height = 0;
		GLbitfield mask = 0;
		GLsizei num_draw_buffers = 0;
		GLenum draw_buffers[FrameBuffer::MAX_COLOR_ATTACHMENTS];
	};

	void fetchGraphicsPipeline();
	void flushPipelineState(CommandBuffer *gl_command_buffer);

private:
	GLuint graphics_pipeline_id {GL_NONE};

	PipelineState pipeline_state;
	PipelineStateOverrides pipeline_state_overrides;
	RenderPassState render_pass_state;
	bool pipeline_dirty { true };

	uint8_t push_constants[MAX_PUSH_CONSTANT_SIZE];
	uint8_t push_constants_size {0};
};

}
