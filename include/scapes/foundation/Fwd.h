#pragma once

#include <scapes/foundation/math/Fwd.h>
#include <scapes/3rdparty/rapidjson/fwd.h>

template<typename T> struct TypeTraits;
template <typename T> struct ResourcePipeline;

namespace scapes::foundation
{
	class Log;

	namespace components
	{
		struct Transform;
	}

	namespace game
	{
		struct EntityID;
		struct QueryID;

		class World;
		class Entity;

		template<typename... Components> class Query;
	}

	namespace io
	{
		enum class SeekOrigin : uint8_t;

		class URI;
		class FileSystem;
		class Stream;
	}

	namespace json = rapidjson;
	namespace math = glm;

	namespace render
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

	namespace resources
	{
		// TODO: get rid of this once we'll have fully-fledged classes for timestamps and generations
		typedef uint32_t timestamp_t;
		typedef uint32_t generation_t;

		struct ResourceMetadata;
		template <typename T> class ResourceHandle;

		class ResourceManager;
	}

	namespace shaders
	{
		enum class ShaderILType : uint8_t;
		enum class ShaderType : uint8_t;

		struct ShaderIL;

		class Compiler;
	}
}
