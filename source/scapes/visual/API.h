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

		SCAPES_INLINE MeshHandle getUnitQuad() const final { return unit_quad; }
		SCAPES_INLINE MeshHandle getUnitCube() const final { return unit_cube; }

		TextureHandle createTexture2D(
			foundation::render::Format format,
			uint32_t width,
			uint32_t height,
			uint32_t num_mips,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		TextureHandle createTexture2D(
			foundation::render::Format format,
			uint32_t width,
			uint32_t height,
			ShaderHandle vertex_shader,
			ShaderHandle fragment_shader
		) final;

		TextureHandle createTextureCube(
			foundation::render::Format format,
			uint32_t size,
			uint32_t num_mips,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
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

		MeshHandle createMeshQuad(
			float size
		) final;

		MeshHandle createMeshCube(
			float size
		) final;

		IBLTextureHandle importIBLTexture(
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
		template<typename HandleType>
		struct HandleCompare
		{
			SCAPES_INLINE bool operator()(
				const HandleType &h0,
				const HandleType &h1
			) const
			{
				return h0.get() < h1.get();
			}
		};

	private:
		foundation::resources::ResourceManager *resource_manager {nullptr};
		foundation::game::World *world {nullptr};
		foundation::render::Device *device {nullptr};
		foundation::shaders::Compiler *compiler {nullptr};

		scapes::visual::MeshHandle unit_quad;
		scapes::visual::MeshHandle unit_cube;

		std::map<std::string, TextureHandle> uri_texture_lookup;
		std::map<TextureHandle, std::string, HandleCompare<TextureHandle>> texture_uri_lookup;

		std::map<std::string, ShaderHandle> uri_shader_lookup;
		std::map<ShaderHandle, std::string, HandleCompare<ShaderHandle>> shader_uri_lookup;

		std::vector<TextureHandle> managed_textures;
		std::vector<ShaderHandle> managed_shaders;
		std::vector<MeshHandle> managed_meshes;
		std::vector<RenderMaterialHandle> managed_render_materials;
		std::vector<IBLTextureHandle> managed_ibl_textures;
	};
}
