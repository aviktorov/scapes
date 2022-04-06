#pragma once

#include <scapes/foundation/TypeTraits.h>
#include <scapes/foundation/math/Math.h>
#include <scapes/foundation/render/Device.h>
#include <scapes/foundation/resources/ResourceManager.h>
#include <scapes/foundation/shaders/Compiler.h>

#include <scapes/visual/Fwd.h>

namespace scapes::visual::resources
{
	/*
	 */
	struct IBLTexture
	{
		// TODO: move to skylight
		TextureHandle baked_brdf;

		foundation::render::Texture prefiltered_specular_cubemap {SCAPES_NULL_HANDLE};
		foundation::render::Texture diffuse_irradiance_cubemap {SCAPES_NULL_HANDLE};

		foundation::render::BindSet bindings {SCAPES_NULL_HANDLE};
		foundation::render::Device *device {nullptr};
	};

	template <>
	struct ::TypeTraits<IBLTexture>
	{
		static constexpr const char *name = "scapes::visual::resources::IBLTexture";
	};

	template <>
	struct ::ResourceTraits<IBLTexture>
	{
		static SCAPES_API size_t size();
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			foundation::render::Device *device
		);
		static SCAPES_API foundation::resources::hash_t fetchHash(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API bool reload(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API void destroy(
			foundation::resources::ResourceManager *resource_manager,
			void *memory
		);
	};

	/*
	 */
	struct Mesh
	{
		struct Vertex
		{
			foundation::math::vec3 position;
			foundation::math::vec4 tangent;
			foundation::math::vec3 binormal;
			foundation::math::vec3 normal;
			foundation::math::vec4 color;
			foundation::math::vec2 uv;
		};

		uint32_t num_vertices {0};
		Vertex *vertices {nullptr};

		uint32_t num_indices {0};
		uint32_t *indices {nullptr};

		foundation::render::VertexBuffer vertex_buffer {SCAPES_NULL_HANDLE};
		foundation::render::IndexBuffer index_buffer {SCAPES_NULL_HANDLE};
		foundation::render::Device *device {nullptr};
	};

	template <>
	struct ::TypeTraits<Mesh>
	{
		static constexpr const char *name = "scapes::visual::resources::Mesh";
	};

	template <>
	struct ::ResourceTraits<Mesh>
	{
		static SCAPES_API size_t size();
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			foundation::render::Device *device
		);
		static SCAPES_API foundation::resources::hash_t fetchHash(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API bool reload(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API void destroy(
			foundation::resources::ResourceManager *resource_manager,
			void *memory
		);
	};

	/*
	 */
	struct RenderMaterial
	{
		TextureHandle albedo;
		TextureHandle normal;
		TextureHandle roughness;
		TextureHandle metalness;
		foundation::render::BindSet bindings {SCAPES_NULL_HANDLE};
		foundation::render::Device *device {nullptr};
	};

	template <>
	struct ::TypeTraits<RenderMaterial>
	{
		static constexpr const char *name = "scapes::visual::resources::RenderMaterial";
	};

	template <>
	struct ::ResourceTraits<RenderMaterial>
	{
		static SCAPES_API size_t size();
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			TextureHandle albedo,
			TextureHandle normal,
			TextureHandle roughness,
			TextureHandle metalness,
			foundation::render::Device *device
		);
		static SCAPES_API foundation::resources::hash_t fetchHash(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API bool reload(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API void destroy(
			foundation::resources::ResourceManager *resource_manager,
			void *memory
		);
	};

	/*
	 */
	struct Shader
	{
		foundation::render::Shader shader {SCAPES_NULL_HANDLE};
		foundation::render::ShaderType type {foundation::render::ShaderType::FRAGMENT};
		foundation::render::Device *device {nullptr};
		foundation::shaders::Compiler *compiler {nullptr};
	};

	template <>
	struct ::TypeTraits<Shader>
	{
		static constexpr const char *name = "scapes::visual::resources::Shader";
	};

	template <>
	struct ::ResourceTraits<Shader>
	{
		static SCAPES_API size_t size();
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			foundation::render::ShaderType type,
			foundation::render::Device *device,
			foundation::shaders::Compiler *compiler
		);
		static SCAPES_API void destroy(
			foundation::resources::ResourceManager *resource_manager,
			void *memory
		);
		static SCAPES_API foundation::resources::hash_t fetchHash(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API bool reload(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API bool loadFromMemory(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			const uint8_t *data,
			size_t size
		);
	};

	/*
	 */
	struct Texture
	{
		uint32_t width {0};
		uint32_t height {0};
		uint32_t mip_levels {0};
		uint32_t layers {0};
		foundation::render::Format format {foundation::render::Format::UNDEFINED};

		unsigned char *cpu_data {nullptr};
		foundation::render::Texture gpu_data {SCAPES_NULL_HANDLE};
		foundation::render::Device *device {nullptr};
	};

	template <>
	struct ::TypeTraits<Texture>
	{
		static constexpr const char *name = "scapes::visual::resources::Texture";
	};

	template <>
	struct ::ResourceTraits<Texture>
	{
		static SCAPES_API size_t size();
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			foundation::render::Device *device
		);
		static SCAPES_API void destroy(
			foundation::resources::ResourceManager *resource_manager,
			void *memory
		);
		static SCAPES_API foundation::resources::hash_t fetchHash(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API bool reload(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API bool loadFromMemory(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			const uint8_t *data,
			size_t size
		);
	};
}
