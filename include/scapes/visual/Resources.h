#pragma once

#include <common/Math.h>
#include <common/Type.h>
#include <scapes/visual/Fwd.h>

#include <render/backend/Driver.h>
#include <render/shaders/Compiler.h>

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
		render::backend::BindSet *bindings {nullptr};
	};

	template <>
	struct ::TypeTraits<IBLTexture>
	{
		static constexpr const char *name = "scapes::visual::resources::IBLTexture";
	};

	template <>
	struct ::ResourcePipeline<IBLTexture>
	{
		static SCAPES_API void destroy(::ResourceManager *resource_manager, IBLTextureHandle handle, render::backend::Driver *driver);
	};

	/*
	 */
	struct Mesh
	{
		struct Vertex
		{
			glm::vec3 position;
			glm::vec4 tangent;
			glm::vec3 binormal;
			glm::vec3 normal;
			glm::vec4 color;
			glm::vec2 uv;
		};

		uint32_t num_vertices {0};
		Vertex *vertices {nullptr};

		uint32_t num_indices {0};
		uint32_t *indices {nullptr};

		render::backend::VertexBuffer *vertex_buffer {nullptr};
		render::backend::IndexBuffer *index_buffer {nullptr};
	};

	template <>
	struct ::TypeTraits<Mesh>
	{
		static constexpr const char *name = "scapes::visual::resources::Mesh";
	};

	template <>
	struct ::ResourcePipeline<Mesh>
	{
		static SCAPES_API void destroy(::ResourceManager *resource_manager, MeshHandle handle, render::backend::Driver *driver);
	};

	/*
	 */
	struct RenderMaterial
	{
		TextureHandle albedo;
		TextureHandle normal;
		TextureHandle roughness;
		TextureHandle metalness;
		::render::backend::BindSet *bindings {nullptr};
	};

	template <>
	struct ::TypeTraits<RenderMaterial>
	{
		static constexpr const char *name = "scapes::visual::resources::RenderMaterial";
	};

	template <>
	struct ::ResourcePipeline<RenderMaterial>
	{
		static SCAPES_API void destroy(::ResourceManager *resource_manager, RenderMaterialHandle handle, render::backend::Driver *driver);
	};

	/*
	 */
	struct Shader
	{
		render::backend::Shader *shader {nullptr};
		render::backend::ShaderType type {render::backend::ShaderType::FRAGMENT};
	};

	template <>
	struct ::TypeTraits<Shader>
	{
		static constexpr const char *name = "scapes::visual::resources::Shader";
	};

	template <>
	struct ::ResourcePipeline<Shader>
	{
		static SCAPES_API void destroy(::ResourceManager *resource_manager, ShaderHandle handle, render::backend::Driver *driver);
		static SCAPES_API bool process(::ResourceManager *resource_manager, ShaderHandle handle, const uint8_t *data, size_t size, render::backend::ShaderType type, render::backend::Driver *driver, render::shaders::Compiler *compiler);
	};

	/*
	 */
	struct Texture
	{
		uint32_t width {0};
		uint32_t height {0};
		uint32_t mip_levels {0};
		uint32_t layers {0};
		render::backend::Format format {render::backend::Format::UNDEFINED};

		unsigned char *cpu_data {nullptr};
		render::backend::Texture *gpu_data {nullptr};
	};

	template <>
	struct ::TypeTraits<Texture>
	{
		static constexpr const char *name = "scapes::visual::resources::Texture";
	};

	template <>
	struct ::ResourcePipeline<Texture>
	{
		static SCAPES_API void destroy(::ResourceManager *resource_manager, TextureHandle handle, render::backend::Driver *driver);
		static SCAPES_API bool process(::ResourceManager *resource_manager, TextureHandle handle, const uint8_t *data, size_t size, render::backend::Driver *driver);
	};
}
