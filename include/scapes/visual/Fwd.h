#pragma once

#include <scapes/foundation/Fwd.h>

namespace scapes::visual
{
	class GlbImporter;
	class HdriImporter;

	class RenderGraph;
	class Material;

	struct IBLTexture;
	struct Mesh;
	struct Shader;
	struct Texture;

	typedef foundation::resources::ResourceHandle<IBLTexture> IBLTextureHandle;
	typedef foundation::resources::ResourceHandle<Mesh> MeshHandle;
	typedef foundation::resources::ResourceHandle<Material> MaterialHandle;
	typedef foundation::resources::ResourceHandle<Shader> ShaderHandle;
	typedef foundation::resources::ResourceHandle<Texture> TextureHandle;
	typedef foundation::resources::ResourceHandle<RenderGraph> RenderGraphHandle;

	namespace components
	{
		struct Transform;
		struct SkyLight;
		struct Renderable;
	}

	namespace hardware
	{
		enum class Api : uint8_t;
		enum class BufferType : uint8_t;
		enum class RenderPrimitiveType : uint8_t;
		enum class IndexFormat : uint8_t;
		enum class Multisample : uint8_t;
		enum class SamplerWrapMode : uint8_t;
		enum class Format : uint16_t;
		enum class RenderPassLoadOp : uint8_t;
		enum class RenderPassStoreOp : uint8_t;
		enum class ShaderILType : uint8_t;
		enum class ShaderType : uint8_t;
		enum class CommandBufferType : uint8_t;
		enum class CullMode : uint8_t;
		enum class DepthCompareFunc : uint8_t;
		enum class BlendFactor : uint8_t;

		struct VertexBuffer_t;
		struct IndexBuffer_t;
		struct Texture_t;
		struct FrameBuffer_t;
		struct RenderPass_t;
		struct CommandBuffer_t;
		struct UniformBuffer_t;
		struct Shader_t;
		struct BindSet_t;
		struct GraphicsPipeline_t;
		struct SwapChain_t;

		typedef struct VertexBuffer_t *VertexBuffer;
		typedef struct IndexBuffer_t *IndexBuffer;
		typedef struct Texture_t *Texture;
		typedef struct FrameBuffer_t *FrameBuffer;
		typedef struct RenderPass_t *RenderPass;
		typedef struct CommandBuffer_t *CommandBuffer;
		typedef struct UniformBuffer_t *UniformBuffer;
		typedef struct Shader_t *Shader;
		typedef struct BindSet_t *BindSet;
		typedef struct GraphicsPipeline_t *GraphicsPipeline;
		typedef struct SwapChain_t *SwapChain;

		struct VertexAttribute;
		struct FrameBufferAttachment;
		union RenderPassClearColor;
		struct RenderPassClearDepthStencil;
		union RenderPassClearValue;
		struct RenderPassAttachment;
		struct RenderPassDescription;

		class Device;
	}

	namespace shaders
	{
		enum class ShaderILType : uint8_t;
		enum class ShaderType : uint8_t;

		struct ShaderIL;

		class Compiler;
	}
}
