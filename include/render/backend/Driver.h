#pragma once

#include <common/Common.h>

namespace render::backend
{
	enum class Api : uint8_t
	{
		VULKAN = 0,
		DEFAULT = VULKAN,

		MAX,
	};

	enum class BufferType : uint8_t
	{
		STATIC = 0,
		DYNAMIC,

		MAX,
	};

	enum class RenderPrimitiveType : uint8_t
	{
		POINT_LIST = 0,
		LINE_LIST,
		LINE_STRIP,
		LINE_PATCH,
		TRIANGLE_LIST,
		TRIANGLE_STRIP,
		TRIANGLE_FAN,
		TRIANGLE_PATCH,
		QUAD_PATCH,

		MAX,
	};

	enum class IndexFormat : uint8_t
	{
		UINT16 = 0,
		UINT32,

		MAX,
	};

	enum class Multisample : uint8_t
	{
		COUNT_1 = 0,
		COUNT_2,
		COUNT_4,
		COUNT_8,
		COUNT_16,
		COUNT_32,
		COUNT_64,

		MAX,
	};

	enum class SamplerWrapMode
	{
		REPEAT,
		CLAMP_TO_EDGE,

		MAX,
	};

	enum class Format : uint16_t
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

		B8G8R8_UNORM,
		B8G8R8_SNORM,
		B8G8R8_UINT,
		B8G8R8_SINT,

		R8G8B8A8_UNORM,
		R8G8B8A8_SNORM,
		R8G8B8A8_UINT,
		R8G8B8A8_SINT,

		B8G8R8A8_UNORM,
		B8G8R8A8_SNORM,
		B8G8R8A8_UINT,
		B8G8R8A8_SINT,

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
		D24_UNORM,
		D24_UNORM_S8_UINT,
		D32_SFLOAT,
		D32_SFLOAT_S8_UINT,

		MAX,
	};

	enum class RenderPassLoadOp : uint8_t
	{
		LOAD = 0,
		CLEAR,
		DONT_CARE,

		MAX,
	};

	enum class RenderPassStoreOp : uint8_t
	{
		STORE = 0,
		DONT_CARE,

		MAX,
	};

	enum class ShaderILType : uint8_t
	{
		SPIRV = 0,
		DEFAULT = SPIRV,

		MAX,
	};

	enum class ShaderType : uint8_t
	{
		// Graphics pipeline
		VERTEX = 0,
		TESSELLATION_CONTROL,
		TESSELLATION_EVALUATION,
		GEOMETRY,
		FRAGMENT,

		// Compute pipeline
		COMPUTE,

		// Raytracing pipeline
		RAY_GENERATION,
		INTERSECTION,
		ANY_HIT,
		CLOSEST_HIT,
		MISS,
		CALLABLE,

		MAX,
	};

	enum class CommandBufferType : uint8_t
	{
		PRIMARY = 0,
		SECONDARY,
	};

	enum class CullMode : uint8_t
	{
		NONE = 0,
		FRONT,
		BACK,
		FRONT_AND_BACK,

		MAX,
	};

	enum class DepthCompareFunc : uint8_t
	{
		NEVER = 0,
		LESS,
		EQUAL,
		LESS_OR_EQUAL,
		GREATER,
		NOT_EQUAL,
		GREATER_OR_EQUAL,
		ALWAYS,

		MAX,
	};

	enum class BlendFactor : uint8_t
	{
		ZERO = 0,
		ONE,
		SRC_COLOR,
		ONE_MINUS_SRC_COLOR,
		DST_COLOR,
		ONE_MINUS_DST_COLOR,
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		DST_ALPHA,
		ONE_MINUS_DST_ALPHA,

		MAX,
	};

	// C opaque structs
	struct VertexBuffer {};
	struct IndexBuffer {};

	struct Texture {};
	struct FrameBuffer {};
	struct RenderPass {};
	struct CommandBuffer {};

	struct UniformBuffer {};
	// TODO: shader storage buffer?

	struct Shader {};
	struct BindSet {};
	struct PipelineState {};

	struct SwapChain {};

	// C structs
	struct VertexAttribute
	{
		Format format {Format::UNDEFINED};
		unsigned int offset {0};
	};

	struct FrameBufferAttachment
	{
		const Texture *texture {nullptr};
		uint32_t base_mip {0};
		uint32_t base_layer {0};
		uint32_t num_layers {1};
	};

	union RenderPassClearColor
	{
		float as_f32[4];
		int32_t as_i32[4];
		uint32_t as_ui32[4];
	};

	struct RenderPassClearDepthStencil
	{
		float depth;
		uint32_t stencil;
	};

	union RenderPassClearValue
	{
		RenderPassClearColor as_color;
		RenderPassClearDepthStencil as_depth_stencil;
	};

	struct RenderPassAttachment
	{
		Format format {Format::UNDEFINED};
		Multisample samples {Multisample::COUNT_1};
		RenderPassLoadOp load_op {RenderPassLoadOp::DONT_CARE};
		RenderPassStoreOp store_op {RenderPassStoreOp::DONT_CARE};
		RenderPassClearValue clear_value;
	};

	struct RenderPassDescription
	{
		uint32_t num_color_attachments {0};
		uint32_t *color_attachments {nullptr};
		uint32_t *resolve_attachments {nullptr};
		uint32_t *depthstencil_attachment {nullptr};
	};

	// main backend class
	class Driver
	{
	public:
		static SCAPES_API Driver *create(const char *application_name, const char *engine_name, Api api = Api::DEFAULT);
		static SCAPES_API void destroy(Driver *driver);

		virtual ~Driver() { }

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
			IndexFormat index_format,
			uint32_t num_indices,
			const void *data
		) = 0;

		virtual Texture *createTexture2D(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) = 0;

		virtual Texture *createTexture2DMultisample(
			uint32_t width,
			uint32_t height,
			Format format,
			Multisample samples
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
			uint32_t size,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) = 0;

		virtual FrameBuffer *createFrameBuffer(
			uint32_t num_attachments,
			const FrameBufferAttachment *attachments
		) = 0;

		virtual RenderPass *createRenderPass(
			uint32_t num_attachments,
			const RenderPassAttachment *attachments,
			const RenderPassDescription &description
		) = 0;

		virtual RenderPass *createRenderPass(
			const SwapChain *swap_chain,
			RenderPassLoadOp load_op,
			RenderPassStoreOp store_op,
			const RenderPassClearColor *clear_color
		) = 0;

		virtual CommandBuffer *createCommandBuffer(
			CommandBufferType type
		) = 0;

		virtual UniformBuffer *createUniformBuffer(
			BufferType type,
			uint32_t size,
			const void *data = nullptr
		) = 0;

		virtual Shader *createShaderFromSource(
			ShaderType type,
			uint32_t size,
			const char *data,
			const char *path = nullptr
		) = 0;

		virtual Shader *createShaderFromIL(
			ShaderType type,
			ShaderILType il_type,
			size_t size,
			const void *data
		) = 0;

		virtual BindSet *createBindSet(
		) = 0;

		virtual PipelineState *createPipelineState(
		) = 0;

		virtual SwapChain *createSwapChain(
			void *native_window
		) = 0;

		virtual void destroyVertexBuffer(VertexBuffer *vertex_buffer) = 0;
		virtual void destroyIndexBuffer(IndexBuffer *index_buffer) = 0;
		virtual void destroyTexture(Texture *texture) = 0;
		virtual void destroyFrameBuffer(FrameBuffer *frame_buffer) = 0;
		virtual void destroyRenderPass(RenderPass *render_pass) = 0;
		virtual void destroyCommandBuffer(CommandBuffer *command_buffer) = 0;
		virtual void destroyUniformBuffer(UniformBuffer *uniform_buffer) = 0;
		virtual void destroyShader(Shader *shader) = 0;
		virtual void destroyBindSet(BindSet *bind_set) = 0;
		virtual void destroyPipelineState(PipelineState *pipeline_state) = 0;
		virtual void destroySwapChain(SwapChain *swap_chain) = 0;

	public:
		virtual bool isFlipped() = 0;
		virtual Multisample getMaxSampleCount() = 0;

		virtual uint32_t getNumSwapChainImages(const SwapChain *swap_chain) = 0;

		virtual void setTextureSamplerWrapMode(Texture *texture, SamplerWrapMode mode) = 0;
		virtual void setTextureSamplerDepthCompare(Texture *texture, bool enabled, DepthCompareFunc func) = 0;
		virtual void generateTexture2DMipmaps(Texture *texture) = 0;

	public:
		virtual void *map(VertexBuffer *vertex_buffer) = 0;
		virtual void unmap(VertexBuffer *vertex_buffer) = 0;

		virtual void *map(IndexBuffer *index_buffer) = 0;
		virtual void unmap(IndexBuffer *index_buffer) = 0;

		virtual void *map(UniformBuffer *uniform_buffer) = 0;
		virtual void unmap(UniformBuffer *uniform_buffer) = 0;

		virtual void flush(BindSet *bind_set) = 0;
		virtual void flush(PipelineState *pipeline_state) = 0;

	public:
		virtual bool acquire(
			SwapChain *swap_chain,
			uint32_t *new_image
		) = 0;

		virtual bool present(
			SwapChain *swap_chain,
			uint32_t num_wait_command_buffers,
			CommandBuffer * const *wait_command_buffers
		) = 0;

		virtual void wait() = 0;
		virtual bool wait(
			uint32_t num_wait_command_buffers,
			CommandBuffer * const *wait_command_buffers
		) = 0;
	public:
		// bindings
		virtual void bindUniformBuffer(
			BindSet *bind_set,
			uint32_t binding,
			const UniformBuffer *uniform_buffer
		) = 0;

		virtual void bindTexture(
			BindSet *bind_set,
			uint32_t binding,
			const Texture *texture
		) = 0;

		virtual void bindTexture(
			BindSet *bind_set,
			uint32_t binding,
			const Texture *texture,
			uint32_t base_mip,
			uint32_t num_mips,
			uint32_t base_layer,
			uint32_t num_layers
		) = 0;

	public:
		// pipeline state
		virtual void clearPushConstants(
			PipelineState *pipeline_state
		) = 0;

		virtual void setPushConstants(
			PipelineState *pipeline_state,
			uint8_t size,
			const void *data
		) = 0;

		virtual void clearBindSets(
			PipelineState *pipeline_state
		) = 0;

		virtual void setBindSet(
			PipelineState *pipeline_state,
			uint8_t binding,
			BindSet *bind_set
		) = 0;

		virtual void clearShaders(
			PipelineState *pipeline_state
		) = 0;

		virtual void setShader(
			PipelineState *pipeline_state,
			ShaderType type,
			const Shader *shader
		) = 0;

		virtual void clearVertexStreams(
			PipelineState *pipeline_state
		) = 0;

		virtual void setVertexStream(
			PipelineState *pipeline_state,
			uint8_t binding,
			VertexBuffer *vertex_buffer
			// TODO: vertex stream usage (per vertex or per instance)
		) = 0;

		virtual void setViewport(
			PipelineState *pipeline_state,
			int32_t x,
			int32_t y,
			uint32_t width,
			uint32_t height
		) = 0;

		virtual void setScissor(
			PipelineState *pipeline_state,
			int32_t x,
			int32_t y,
			uint32_t width,
			uint32_t height
		) = 0;

		virtual void setPrimitiveType(
			PipelineState *pipeline_state,
			RenderPrimitiveType type
		) = 0;

		virtual void setCullMode(
			PipelineState *pipeline_state,
			CullMode mode
		) = 0;

		virtual void setDepthTest(
			PipelineState *pipeline_state,
			bool enabled
		) = 0;

		virtual void setDepthWrite(
			PipelineState *pipeline_state,
			bool enabled
		) = 0;

		virtual void setDepthCompareFunc(
			PipelineState *pipeline_state,
			DepthCompareFunc func
		) = 0;

		virtual void setBlending(
			PipelineState *pipeline_state,
			bool enabled
		) = 0;

		virtual void setBlendFactors(
			PipelineState *pipeline_state,
			BlendFactor src_factor,
			BlendFactor dest_factor
		) = 0;

	public:
		// command buffers
		virtual bool resetCommandBuffer(
			CommandBuffer *command_buffer
		) = 0;

		virtual bool beginCommandBuffer(
			CommandBuffer *command_buffer
		) = 0;

		virtual bool endCommandBuffer(
			CommandBuffer *command_buffer
		) = 0;

		virtual bool submit(
			CommandBuffer *command_buffer
		) = 0;

		virtual bool submitSyncked(
			CommandBuffer *command_buffer,
			const SwapChain *wait_swap_chain
		) = 0;

		virtual bool submitSyncked(
			CommandBuffer *command_buffer,
			uint32_t num_wait_command_buffers,
			CommandBuffer * const *wait_command_buffers
		) = 0;
	public:
		// render commands
		virtual void beginRenderPass(
			CommandBuffer *command_buffer,
			const RenderPass *render_pass,
			const FrameBuffer *frame_buffer
		) = 0;

		virtual void beginRenderPass(
			CommandBuffer *command_buffer,
			const RenderPass *render_pass,
			const SwapChain *swap_chain
		) = 0;

		virtual void endRenderPass(
			CommandBuffer *command_buffer
		) = 0;

		virtual void drawIndexedPrimitiveInstanced(
			CommandBuffer *command_buffer,
			PipelineState *pipeline_state,
			const IndexBuffer *index_buffer,
			uint32_t num_indices,
			uint32_t base_index = 0,
			int32_t base_vertex = 0,
			uint32_t num_instances = 1,
			uint32_t base_instance = 0
		) = 0;
	};
}
