#pragma once

#include <scapes/foundation/components/Components.h>

#include <scapes/visual/API.h>
#include <scapes/visual/Components.h>
#include <scapes/visual/Resources.h>

#include <vector>
#include <map>
#include <string>

namespace scapes::visual
{
	class APIImpl : public API
	{
	public:
		APIImpl(
			const foundation::io::URI &default_vertex_shader_uri,
			const foundation::io::URI &default_cubemap_geometry_shader_uri,
			foundation::resources::ResourceManager *resource_manager,
			foundation::game::World *world,
			foundation::render::Device *device,
			foundation::shaders::Compiler *compiler
		);

		~APIImpl();

		SCAPES_INLINE foundation::resources::ResourceManager *getResourceManager() const final { return resource_manager; }
		SCAPES_INLINE foundation::game::World *getWorld() const final { return world; }
		SCAPES_INLINE foundation::render::Device *getDevice() const final { return device; }
		SCAPES_INLINE foundation::shaders::Compiler *getCompiler() const final { return compiler; }

		SCAPES_INLINE ShaderHandle getDefaultVertexShader() const final { return default_vertex; }

		SCAPES_INLINE MeshHandle getUnitQuad() const final { return unit_quad; }
		SCAPES_INLINE MeshHandle getUnitCube() const final { return unit_cube; }

		SCAPES_INLINE TextureHandle getWhiteTexture() const final { return default_white; }
		SCAPES_INLINE TextureHandle getGreyTexture() const final { return default_grey; }
		SCAPES_INLINE TextureHandle getBlackTexture() const final { return default_black; }
		SCAPES_INLINE TextureHandle getNormalTexture() const final { return default_normal; }

		TextureHandle createTexture2D(
			foundation::render::Format format,
			uint32_t width,
			uint32_t height,
			uint32_t num_mips,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		void renderTexture2D(
			TextureHandle target,
			ShaderHandle fragment_shader
		) final;

		TextureHandle createTextureCube(
			foundation::render::Format format,
			uint32_t size,
			uint32_t num_mips,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		void renderTextureCube(
			TextureHandle target,
			uint32_t target_mip,
			ShaderHandle fragment_shader,
			TextureHandle input_texture,
			size_t size = 0,
			const void *data = nullptr
		) final;

		TextureHandle loadTextureFromMemory(
			const uint8_t *data,
			size_t size
		) final;

		TextureHandle loadTexture(
			const foundation::io::URI &uri
		) final;

		foundation::io::URI getTextureUri(
			TextureHandle handle
		) const final;

		ShaderHandle loadShaderFromMemory(
			const uint8_t *data,
			size_t size,
			foundation::render::ShaderType shader_type
		) final;

		ShaderHandle loadShader(
			const foundation::io::URI &uri,
			foundation::render::ShaderType shader_type
		) final;

		foundation::io::URI getShaderUri(
			ShaderHandle handle
		) const final;

		MeshHandle createMesh(
			uint32_t num_vertices,
			resources::Mesh::Vertex *vertices,
			uint32_t num_indices,
			uint32_t *indices
		) final;

		IBLTextureHandle loadIBLTexture(
			const foundation::io::URI &uri,
			const IBLTextureCreateData &create_data
		) final;

		RenderMaterialHandle createRenderMaterial(
			TextureHandle albedo,
			TextureHandle normal,
			TextureHandle roughness,
			TextureHandle metalness
		) final;

	private:
		MeshHandle generateMeshQuad(float size);
		MeshHandle generateMeshCube(float size);

		TextureHandle generateTexture(uint8_t r, uint8_t g, uint8_t b);

	private:
		foundation::resources::ResourceManager *resource_manager {nullptr};
		foundation::game::World *world {nullptr};
		foundation::render::Device *device {nullptr};
		foundation::shaders::Compiler *compiler {nullptr};

		ShaderHandle default_vertex;
		ShaderHandle default_cubemap_geometry;

		MeshHandle unit_quad;
		MeshHandle unit_cube;

		TextureHandle default_white;
		TextureHandle default_grey;
		TextureHandle default_black;
		TextureHandle default_normal;
	};
}
