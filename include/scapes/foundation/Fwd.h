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
		// TODO: get rid of this once we'll have fully-fledged URI class
		typedef const char * URI;

		enum class SeekOrigin : uint8_t;

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

		struct VertexBuffer;
		struct IndexBuffer;
		struct Texture;
		struct FrameBuffer;
		struct RenderPass;
		struct CommandBuffer;
		struct UniformBuffer;
		struct Shader;
		struct BindSet;
		struct PipelineState;
		struct SwapChain;

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
		typedef int timestamp_t;
		typedef int generation_t;

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
