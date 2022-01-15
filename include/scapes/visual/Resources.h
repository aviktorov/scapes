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

		// TODO: make it explicit that IBL owns this textures (could be just raw render::backend::Texture pointers?)
		TextureHandle prefiltered_specular_cubemap;
		TextureHandle diffuse_irradiance_cubemap;

		foundation::render::BindSet bindings {SCAPES_NULL_HANDLE};
	};

	template <>
	struct ::TypeTraits<IBLTexture>
	{
		static constexpr const char *name = "scapes::visual::resources::IBLTexture";
	};

	template <>
	struct ::ResourcePipeline<IBLTexture>
	{
		static SCAPES_API void destroy(foundation::resources::ResourceManager *resource_manager, IBLTextureHandle handle, foundation::render::Device *device);
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
	};

	template <>
	struct ::TypeTraits<Mesh>
	{
		static constexpr const char *name = "scapes::visual::resources::Mesh";
	};

	template <>
	struct ::ResourcePipeline<Mesh>
	{
		static SCAPES_API void destroy(foundation::resources::ResourceManager *resource_manager, MeshHandle handle, foundation::render::Device *device);
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
	};

	template <>
	struct ::TypeTraits<RenderMaterial>
	{
		static constexpr const char *name = "scapes::visual::resources::RenderMaterial";
	};

	template <>
	struct ::ResourcePipeline<RenderMaterial>
	{
		static SCAPES_API void destroy(foundation::resources::ResourceManager *resource_manager, RenderMaterialHandle handle, foundation::render::Device *device);
	};

	/*
	 */
	struct Shader
	{
		foundation::render::Shader shader {SCAPES_NULL_HANDLE};
		foundation::render::ShaderType type {foundation::render::ShaderType::FRAGMENT};
	};

	template <>
	struct ::TypeTraits<Shader>
	{
		static constexpr const char *name = "scapes::visual::resources::Shader";
	};

	template <>
	struct ::ResourcePipeline<Shader>
	{
		static SCAPES_API void destroy(foundation::resources::ResourceManager *resource_manager, ShaderHandle handle, foundation::render::Device *device);
		static SCAPES_API bool process(foundation::resources::ResourceManager *resource_manager, ShaderHandle handle, const uint8_t *data, size_t size, foundation::render::ShaderType type, foundation::render::Device *device, foundation::shaders::Compiler *compiler);
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
	};

	template <>
	struct ::TypeTraits<Texture>
	{
		static constexpr const char *name = "scapes::visual::resources::Texture";
	};

	template <>
	struct ::ResourcePipeline<Texture>
	{
		static SCAPES_API void destroy(foundation::resources::ResourceManager *resource_manager, TextureHandle handle, foundation::render::Device *device);
		static SCAPES_API bool process(foundation::resources::ResourceManager *resource_manager, TextureHandle handle, const uint8_t *data, size_t size, foundation::render::Device *device);
	};
}
