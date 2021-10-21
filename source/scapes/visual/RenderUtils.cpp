#include "RenderUtils.h"

#include <scapes/visual/Resources.h>

#include <CubemapRenderer.h>
#include <Texture2DRenderer.h>

#include <cassert>

namespace scapes::visual
{
	/*
	 */
	void RenderUtils::uploadToGPU(
		::render::backend::Driver *driver,
		MeshHandle mesh
	)
	{
		assert(driver);
		assert(mesh.get());
		assert(mesh->num_vertices);
		assert(mesh->num_indices);
		assert(mesh->vertices);
		assert(mesh->indices);

		static render::backend::VertexAttribute mesh_attributes[6] =
		{
			{ render::backend::Format::R32G32B32_SFLOAT, offsetof(resources::Mesh::Vertex, position) },
			{ render::backend::Format::R32G32_SFLOAT, offsetof(resources::Mesh::Vertex, uv) },
			{ render::backend::Format::R32G32B32A32_SFLOAT, offsetof(resources::Mesh::Vertex, tangent) },
			{ render::backend::Format::R32G32B32_SFLOAT, offsetof(resources::Mesh::Vertex, binormal) },
			{ render::backend::Format::R32G32B32_SFLOAT, offsetof(resources::Mesh::Vertex, normal) },
			{ render::backend::Format::R32G32B32A32_SFLOAT, offsetof(resources::Mesh::Vertex, color) },
		};

		mesh->vertex_buffer = driver->createVertexBuffer(
			render::backend::BufferType::STATIC,
			sizeof(resources::Mesh::Vertex), mesh->num_vertices,
			6, mesh_attributes,
			mesh->vertices
		);

		mesh->index_buffer = driver->createIndexBuffer(
			render::backend::BufferType::STATIC,
			render::backend::IndexFormat::UINT32,
			mesh->num_indices,
			mesh->indices
		);
	}

	/*
	 */
	void RenderUtils::renderTexture2D(
		::render::backend::Driver *driver,
		TextureHandle result,
		MeshHandle fullscreen_quad,
		ShaderHandle vertex_shader,
		ShaderHandle fragment_shader
	)
	{
		Texture2DRenderer renderer(driver);
		renderer.init(result.get());
		renderer.render(fullscreen_quad.get(), vertex_shader.get(), fragment_shader.get());
	}

	void RenderUtils::renderTextureCube(
		::render::backend::Driver *driver,
		TextureHandle result,
		MeshHandle fullscreen_quad,
		ShaderHandle vertex_shader,
		ShaderHandle fragment_shader,
		TextureHandle input
	)
	{
		CubemapRenderer renderer(driver);
		renderer.init(result.get(), 0);
		renderer.render(fullscreen_quad.get(), vertex_shader.get(), fragment_shader.get(), input.get());
	}

	/*
	 */
	void RenderUtils::renderHDRIToCube(
		::render::backend::Driver *driver,
		TextureHandle result,
		MeshHandle fullscreen_quad,
		ShaderHandle vertex_shader,
		ShaderHandle hdri_fragment_shader,
		ShaderHandle prefilter_fragment_shader,
		TextureHandle hdri,
		TextureHandle temp_cubemap
	)
	{
		CubemapRenderer renderer(driver);
		renderer.init(temp_cubemap.get(), 0);
		renderer.render(fullscreen_quad.get(), vertex_shader.get(), hdri_fragment_shader.get(), hdri.get());

		for (uint32_t mip = 0; mip < result->mip_levels; ++mip)
		{
			float roughness = static_cast<float>(mip) / result->mip_levels;

			uint8_t size = static_cast<uint8_t>(sizeof(float));
			const uint8_t *data = reinterpret_cast<const uint8_t *>(&roughness);

			CubemapRenderer mip_renderer(driver);
			mip_renderer.init(result.get(), mip);
			mip_renderer.render(fullscreen_quad.get(), vertex_shader.get(), prefilter_fragment_shader.get(), temp_cubemap.get(), size, data);
		}
	}
}
