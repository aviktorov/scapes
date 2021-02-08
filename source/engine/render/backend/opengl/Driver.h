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
	GLuint index_format {0};
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

	GLuint main_fbo_id {0};
	uint8_t num_color_attachments {0};
	FrameBufferColorAttachment color_attachments[FrameBuffer::MAX_COLOR_ATTACHMENTS];
	FrameBufferDepthStencilAttachment depthstencil_attachment;

	GLuint resolve_fbo_id {0};
	uint8_t num_resolve_color_attachments {0};
	FrameBufferColorAttachment resolve_color_attachments[FrameBuffer::MAX_COLOR_ATTACHMENTS];
};

struct Shader : public backend::Shader
{
	GLuint id {0};
	GLint compiled {GL_FALSE};
	GLenum type {GL_FRAGMENT_SHADER};
};

struct UniformBuffer : public backend::UniformBuffer
{
	Buffer *data {nullptr};
};

struct SwapChain : public backend::SwapChain
{
	Surface *surface {nullptr};
	uint32_t num_images {0};
	uint32_t current_image {0};
	bool debug {false};
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
	void makeCurrent(backend::SwapChain *swap_chain);

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
		MAX_TEXTURES = 16,
		MAX_UNIFORM_BUFFERS = 16,
		MAX_SHADERS = static_cast<int>(ShaderType::MAX),
	};

	struct PipelineState
	{
		GLint viewport_x { 0 };
		GLint viewport_y { 0 };
		GLsizei viewport_width { 0 };
		GLsizei viewport_height { 0 };
		uint8_t depth_test : 1;
		uint8_t depth_write : 1;
		GLenum depth_comparison_func { GL_LEQUAL };
		uint8_t blend : 1;
		GLenum blend_src_factor { GL_ZERO };
		GLenum blend_dst_factor { GL_ZERO };
		GLenum cull_mode { GL_NONE };
		GLenum bound_texture_types[MAX_TEXTURES];
		GLuint bound_textures[MAX_TEXTURES];
		GLuint bound_uniform_buffers[MAX_UNIFORM_BUFFERS];
		GLuint bound_shaders[MAX_SHADERS];
	};

	struct PipelineStateOverrides
	{
		uint8_t viewport : 1;
		uint8_t depth_write : 1;
		uint8_t depth_test : 1;
		uint8_t depth_comparison_func : 1;
		uint8_t blend : 1;
		uint8_t blend_factors : 1;
		uint8_t cull_mode: 1;
		uint16_t bound_textures;
		uint16_t bound_uniform_buffers;
		uint16_t bound_shaders;
	};

	void fetchGraphicsPipeline();
	void flushPipelineState();

private:
	GLuint graphics_pipeline_id {GL_NONE};

	PipelineState pipeline_state;
	PipelineStateOverrides pipeline_state_overrides;
	bool pipeline_dirty { true };

	GLuint bound_vao { GL_NONE };
	GLuint bound_ib { GL_NONE };
};

}
