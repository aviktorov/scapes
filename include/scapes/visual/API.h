#pragma once

#include <scapes/visual/Fwd.h>
#include <scapes/visual/Resources.h>

namespace scapes::visual
{
	struct IBLTextureCreateData
	{
		foundation::render::Format format {foundation::render::Format::R32G32B32A32_SFLOAT};
		uint32_t cubemap_size {128};

		TextureHandle baked_brdf;
		ShaderHandle equirectangular_projection_fragment; // TODO: replace graphics pipeline by compute?
		ShaderHandle prefiltered_specular_fragment;
		ShaderHandle diffuse_irradiance_fragment;
	};

	class API
	{
	public:
		static SCAPES_API API *create(
			const foundation::io::URI &default_vertex_shader_uri,
			const foundation::io::URI &default_cubemap_geometry_shader_uri,
			foundation::resources::ResourceManager *resource_manager,
			foundation::game::World *world,
			foundation::render::Device *device,
			foundation::shaders::Compiler *compiler
		);
		static SCAPES_API void destroy(API *api);

		virtual ~API() { }

	public:
		virtual foundation::resources::ResourceManager *getResourceManager() const = 0;
		virtual foundation::game::World *getWorld() const = 0;
		virtual foundation::render::Device *getDevice() const = 0;
		virtual foundation::shaders::Compiler *getCompiler() const = 0;

		virtual ShaderHandle getDefaultVertexShader() const = 0;

		virtual MeshHandle getUnitQuad() const = 0;
		virtual MeshHandle getUnitCube() const = 0;

		virtual TextureHandle getWhiteTexture() const = 0;
		virtual TextureHandle getGreyTexture() const = 0;
		virtual TextureHandle getBlackTexture() const = 0;
		virtual TextureHandle getNormalTexture() const = 0;

		virtual TextureHandle createTexture2D(
			foundation::render::Format format,
			uint32_t width,
			uint32_t height,
			uint32_t num_mips,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) = 0;

		virtual void renderTexture2D(
			TextureHandle target,
			ShaderHandle fragment_shader
		) = 0;

		virtual TextureHandle createTextureCube(
			foundation::render::Format format,
			uint32_t size,
			uint32_t num_mips,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) = 0;

		virtual void renderTextureCube(
			TextureHandle target,
			uint32_t target_mip,
			ShaderHandle fragment_shader,
			TextureHandle input_texture,
			size_t size = 0,
			const void *data = nullptr
		) = 0;

		virtual MeshHandle createMesh(
			uint32_t num_vertices,
			resources::Mesh::Vertex *vertices,
			uint32_t num_indices,
			uint32_t *indices
		) = 0;

		virtual IBLTextureHandle loadIBLTexture(
			const foundation::io::URI &uri,
			const IBLTextureCreateData &create_data
		) = 0;
	};
}
