#pragma once

#include <cstdint>

namespace render::backend
{

enum class BufferType : unsigned char
{
	STATIC = 0,
	DYNAMIC,
};

enum class RenderPrimitiveType : unsigned char
{
	POINT_LIST = 0,
	LINE_LIST,
	LINE_PATCH,
	LINE_STRIP,
	TRIANGLE_LIST,
	TRIANGLE_PATCH,
	TRIANGLE_STRIP,
	TRIANGLE_FAN,
	QUAD_PATCH,
	MAX
};

enum class IndexSize : unsigned char
{
	UINT16 = 0,
	UINT32,
	MAX
};

enum class Format : unsigned int
{
	UNDEFINED = 0,

	// 8-bit formats
	R8_UNORM,
	R8_SNORM,
	R8_UINT,
	R8_SINT,

	R8G8_UNORM,
	R8G8_SNORM,
	R8G8_UINT,
	R8G8_SINT,

	R8G8B8_UNORM,
	R8G8B8_SNORM,
	R8G8B8_UINT,
	R8G8B8_SINT,

	R8G8B8A8_UNORM,
	R8G8B8A8_SNORM,
	R8G8B8A8_UINT,
	R8G8B8A8_SINT,

	// 16-bit formats
	R16_UNORM,
	R16_SNORM,
	R16_UINT,
	R16_SINT,
	R16_SFLOAT,

	R16G16_UNORM,
	R16G16_SNORM,
	R16G16_UINT,
	R16G16_SINT,
	R16G16_SFLOAT,

	R16G16B16_UNORM,
	R16G16B16_SNORM,
	R16G16B16_UINT,
	R16G16B16_SINT,
	R16G16B16_SFLOAT,

	R16G16B16A16_UNORM,
	R16G16B16A16_SNORM,
	R16G16B16A16_UINT,
	R16G16B16A16_SINT,
	R16G16B16A16_SFLOAT,

	// 32-bit formats
	R32_UINT,
	R32_SINT,
	R32_SFLOAT,

	R32G32_UINT,
	R32G32_SINT,
	R32G32_SFLOAT,

	R32G32B32_UINT,
	R32G32B32_SINT,
	R32G32B32_SFLOAT,

	R32G32B32A32_UINT,
	R32G32B32A32_SINT,
	R32G32B32A32_SFLOAT,

	// depth formats
	D16_UNORM,
	D16_UNORM_S8_UINT,
	D24_UNORM_S8_UINT,
	D32_SFLOAT,
	D32_SFLOAT_S8_UINT,

	MAX,
};

enum class ShaderType : unsigned char
{
	// Graphics pipeline
	Vertex = 0,
	TessellationControl,
	TessellationEvaulation,
	Geometry,
	Fragment,

	// Compute pipeline
	Compute,

	// Raytracing pipeline
	RayGeneration,
	Intersection,
	AnyHit,
	ClosestHit,
	Miss,

	// Misc
	Callable,
};

// C opaque structs
struct VertexBuffer {};
struct IndexBuffer {};
struct RenderPrimitive {};

struct Texture {};
struct FrameBuffer {};
// TODO: render pass?

struct UniformBuffer {};
// TODO: shader storage buffer?

struct Shader {};

// C structs
struct VertexAttribute
{
	Format format {Format::UNDEFINED};
	unsigned int offset {0};
};

struct FrameBufferColorAttachment
{
	const Texture *attachment {nullptr};
	// TODO: add support for load / store operations
	// TODO: add clear values
};

struct FrameBufferDepthStencilAttachment
{
	const Texture *attachment {nullptr};
	// TODO: add support for load / store operations
	// TODO: add support for load / store stencil operations
	// TODO: add clear values
};

// main backend class
class Driver
{
public:
	virtual VertexBuffer *createVertexBuffer(
		BufferType type,
		uint16_t vertex_size,
		uint32_t num_vertices,
		uint8_t num_attributes,
		const VertexAttribute *attributes,
		const void *data
	) = 0;

	virtual IndexBuffer *createIndexBuffer(
		BufferType type,
		IndexSize index_size,
		uint32_t num_indices,
		const void *data
	) = 0;

	virtual RenderPrimitive *createRenderPrimitive(
		RenderPrimitiveType type,
		const VertexBuffer *vertex_buffer,
		const IndexBuffer *index_buffer
	) = 0;

	virtual Texture *createTexture2D(
		uint32_t width,
		uint32_t height,
		uint32_t num_mipmaps,
		Format format,
		const void *data = nullptr,
		uint32_t num_data_mipmaps = 1
	) = 0;

	virtual Texture *createTexture2DArray(
		uint32_t width,
		uint32_t height,
		uint32_t num_mipmaps,
		uint32_t num_layers,
		Format format,
		const void *data = nullptr,
		uint32_t num_data_mipmaps = 1,
		uint32_t num_data_layers = 1
	) = 0;

	virtual Texture *createTexture3D(
		uint32_t width,
		uint32_t height,
		uint32_t depth,
		uint32_t num_mipmaps,
		Format format,
		const void *data = nullptr,
		uint32_t num_data_mipmaps = 1
	) = 0;

	virtual Texture *createTextureCube(
		uint32_t width,
		uint32_t height,
		uint32_t num_mipmaps,
		Format format,
		const void *data = nullptr,
		uint32_t num_data_mipmaps = 1
	) = 0;

	virtual FrameBuffer *createFrameBuffer(
		uint8_t num_color_attachments,
		const FrameBufferColorAttachment *color_attachments,
		const FrameBufferDepthStencilAttachment *depthstencil_attachment = nullptr
	) = 0;

	virtual UniformBuffer *createUniformBuffer(
		BufferType type,
		uint32_t size,
		const void *data
	) = 0;

	virtual Shader *createShaderFromSource(
		ShaderType type,
		uint32_t length,
		const char *source
	) = 0;

	virtual Shader *createShaderFromBytecode(
		ShaderType type,
		uint32_t size,
		const void *data
	) = 0;

	virtual void destroyVertexBuffer(VertexBuffer *vertex_buffer) = 0;
	virtual void destroyIndexBuffer(IndexBuffer *index_buffer) = 0;
	virtual void destroyRenderPrimitive(RenderPrimitive *render_primitive) = 0;
	virtual void destroyTexture(Texture *texture) = 0;
	virtual void destroyFrameBuffer(FrameBuffer *frame_buffer) = 0;
	virtual void destroyUniformBuffer(UniformBuffer *uniform_buffer) = 0;
	virtual void destroyShader(Shader *shader) = 0;

public:

	// sequence
	virtual void beginRenderPass(
		const FrameBuffer *frame_buffer
	) = 0;

	virtual void endRenderPass() = 0;

	// bind
	virtual void bindUniformBuffer(
		uint32_t unit,
		const UniformBuffer *uniform_buffer
	) = 0;

	virtual void bindTexture(
		uint32_t unit,
		const Texture *texture
	) = 0;

	virtual void bindShader(
		const Shader *shader
	) = 0;

	// draw
	virtual void drawIndexedPrimitive(
		const RenderPrimitive *render_primitive
	) = 0;

	virtual void drawIndexedPrimitiveInstanced(
		const RenderPrimitive *primitive,
		const VertexBuffer *instance_buffer,
		uint32_t num_instances,
		uint32_t offset
	) = 0;
};

}
