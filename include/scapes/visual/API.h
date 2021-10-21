#pragma once

#include <scapes/visual/Fwd.h>
#include <scapes/visual/Resources.h>

// TODO: include foundation forward decls instead
#include <game/World.h>
#include <render/backend/Driver.h>

namespace scapes::visual
{
	struct IBLTextureCreateData
	{
		render::backend::Format format {render::backend::Format::R32G32B32A32_SFLOAT};
		uint32_t cubemap_size {128};

		MeshHandle fullscreen_quad; // TODO: find a way to get rid of fullscreen quad mesh
		TextureHandle baked_brdf;
		ShaderHandle cubemap_vertex; // TODO: find a way to get rid of cubemap vertex shader
		ShaderHandle equirectangular_projection_fragment;
		ShaderHandle prefiltered_specular_fragment;
		ShaderHandle diffuse_irradiance_fragment;
	};

	class API
	{
	public:
		static SCAPES_API API *create(::ResourceManager *resource_manager, ::game::World *world, ::render::backend::Driver *driver, ::render::shaders::Compiler *compiler);
		static SCAPES_API void destroy(API *api);

		virtual ~API() { }

	public:

		virtual void drawRenderables(
			uint8_t material_binding,
			::render::backend::PipelineState *pipeline_state,
			::render::backend::CommandBuffer *command_buffer
		) = 0;

		virtual void drawSkyLights(
			uint8_t light_binding,
			::render::backend::PipelineState *pipeline_state,
			::render::backend::CommandBuffer *command_buffer
		) = 0;

		virtual TextureHandle createTexture2D(
			render::backend::Format format,
			uint32_t width,
			uint32_t height,
			uint32_t num_mips,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) = 0;

		virtual TextureHandle createTexture2D(
			render::backend::Format format,
			uint32_t width,
			uint32_t height,
			MeshHandle fullscreen_quad,
			ShaderHandle vertex_shader,
			ShaderHandle fragment_shader
		) = 0;

		virtual TextureHandle createTextureCube(
			render::backend::Format format,
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
			const URI &uri
		) = 0;

		virtual ShaderHandle loadShaderFromMemory(
			const uint8_t *data,
			size_t size,
			render::backend::ShaderType shader_type
		) = 0;

		virtual ShaderHandle loadShader(
			const URI &uri,
			render::backend::ShaderType shader_type
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
			const URI &uri,
			const IBLTextureCreateData &create_data
		) = 0;

		virtual RenderMaterialHandle createRenderMaterial(
			TextureHandle albedo,
			TextureHandle normal,
			TextureHandle roughness,
			TextureHandle metalness
		) = 0;

		/*
		virtual MeshHandle createMeshFromAssimp(const aiMesh *mesh) = 0;
		virtual MeshHandle createMeshFromCGLTF(const cgltf_mesh *mesh) = 0;
		/**/
	};
}
