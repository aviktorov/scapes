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
		ShaderHandle cubemap_vertex; // TODO: find a way to get rid of cubemap vertex shader
		ShaderHandle equirectangular_projection_fragment;
		ShaderHandle prefiltered_specular_fragment;
		ShaderHandle diffuse_irradiance_fragment;
	};

	class API
	{
	public:
		static SCAPES_API API *create(foundation::resources::ResourceManager *resource_manager, foundation::game::World *world, foundation::render::Device *device, foundation::shaders::Compiler *compiler);
		static SCAPES_API void destroy(API *api);

		virtual ~API() { }

	public:
		virtual foundation::resources::ResourceManager *getResourceManager() const = 0;
		virtual foundation::game::World *getWorld() const = 0;
		virtual foundation::render::Device *getDevice() const = 0;
		virtual foundation::shaders::Compiler *getCompiler() const = 0;

		virtual MeshHandle getFullscreenQuadMesh() const = 0;

		virtual TextureHandle createTexture2D(
			foundation::render::Format format,
			uint32_t width,
			uint32_t height,
			uint32_t num_mips,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) = 0;

		virtual TextureHandle createTexture2D(
			foundation::render::Format format,
			uint32_t width,
			uint32_t height,
			ShaderHandle vertex_shader,
			ShaderHandle fragment_shader
		) = 0;

		virtual TextureHandle createTextureCube(
			foundation::render::Format format,
			uint32_t size,
			uint32_t num_mips,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) = 0;

		virtual TextureHandle loadTextureFromMemory(
			const uint8_t *data,
			size_t size
		) = 0;

		virtual TextureHandle loadTexture(
			const foundation::io::URI &uri
		) = 0;

		virtual ShaderHandle loadShaderFromMemory(
			const uint8_t *data,
			size_t size,
			foundation::render::ShaderType shader_type
		) = 0;

		virtual ShaderHandle loadShader(
			const foundation::io::URI &uri,
			foundation::render::ShaderType shader_type
		) = 0;

		virtual MeshHandle createMesh(
			uint32_t num_vertices,
			resources::Mesh::Vertex *vertices,
			uint32_t num_indices,
			uint32_t *indices
		) = 0;

		virtual MeshHandle createMeshQuad(
			float size
		) = 0;

		virtual MeshHandle createMeshSkybox(
			float size
		) = 0;

		virtual IBLTextureHandle importIBLTexture(
			const foundation::io::URI &uri,
			const IBLTextureCreateData &create_data
		) = 0;

		virtual RenderMaterialHandle createRenderMaterial(
			TextureHandle albedo,
			TextureHandle normal,
			TextureHandle roughness,
			TextureHandle metalness
		) = 0;
	};
}
