#include <scapes/visual/API.h>
#include <scapes/visual/Components.h>
#include <scapes/visual/Resources.h>

#include <vector>

namespace scapes::visual
{
	class APIImpl : public API
	{
	public:
		APIImpl(::ResourceManager *resource_manager, ::game::World *world, ::render::backend::Driver *driver, ::render::shaders::Compiler *compiler);
		~APIImpl();

		void drawRenderables(
			uint8_t material_binding,
			::render::backend::PipelineState *pipeline_state,
			::render::backend::CommandBuffer *command_buffer
		) final;

		void drawSkyLights(
			uint8_t light_binding,
			::render::backend::PipelineState *pipeline_state,
			::render::backend::CommandBuffer *command_buffer
		) final;

		TextureHandle createTexture2D(
			render::backend::Format format,
			uint32_t width,
			uint32_t height,
			uint32_t num_mips,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		TextureHandle createTexture2D(
			render::backend::Format format,
			uint32_t width,
			uint32_t height,
			MeshHandle fullscreen_quad,
			ShaderHandle vertex_shader,
			ShaderHandle fragment_shader
		) final;

		TextureHandle createTextureCube(
			render::backend::Format format,
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
			const URI &uri
		) final;

		ShaderHandle loadShaderFromMemory(
			const uint8_t *data,
			size_t size,
			render::backend::ShaderType shader_type
		) final;

		ShaderHandle loadShader(
			const URI &uri,
			render::backend::ShaderType shader_type
		) final;

		MeshHandle createMesh(
			uint32_t num_vertices,
			resources::Mesh::Vertex *vertices,
			uint32_t num_indices,
			uint32_t *indices
		) final;

		MeshHandle createMeshQuad(
			float size
		) final;

		MeshHandle createMeshSkybox(
			float size
		) final;

		IBLTextureHandle importIBLTexture(
			const URI &uri,
			const IBLTextureCreateData &create_data
		) final;

		RenderMaterialHandle createRenderMaterial(
			TextureHandle albedo,
			TextureHandle normal,
			TextureHandle roughness,
			TextureHandle metalness
		) final;

	private:
		::ResourceManager *resource_manager {nullptr};
		::game::World *world {nullptr};
		::render::backend::Driver *driver {nullptr};
		::render::shaders::Compiler *compiler {nullptr};

		std::vector<TextureHandle> managed_textures;
		std::vector<ShaderHandle> managed_shaders;
		std::vector<MeshHandle> managed_meshes;
		std::vector<RenderMaterialHandle> managed_render_materials;
		std::vector<IBLTextureHandle> managed_ibl_textures;

		::game::Query<components::Transform, components::Renderable> *renderable_query {nullptr};
		::game::Query<components::SkyLight> *skylight_query {nullptr};
	};
}
