#pragma once

#include <render/backend/Driver.h>
#include <common/ResourceManager.h>

struct Mesh;
struct Shader;
struct Texture;

/*
 */
struct IBLTexture
{
	// TODO: move to skylight
	resources::ResourceHandle<Texture> baked_brdf;

	// TODO: make it explicit that IBL owns this textures (could be just raw render::backend::Texture pointers?)
	resources::ResourceHandle<Texture> prefiltered_specular_cubemap;
	resources::ResourceHandle<Texture> diffuse_irradiance_cubemap;
	render::backend::BindSet *bindings {nullptr};
};

template <>
struct TypeTraits<IBLTexture>
{
	static constexpr const char *name = "IBLTexture";
};

template <>
struct resources::ResourcePipeline<IBLTexture>
{
	struct ImportData
	{
		render::backend::Format format {render::backend::Format::R32G32B32A32_SFLOAT};
		uint32_t cubemap_size {128};

		// TODO: find a way to get rid of fullscreen quad mesh
		resources::ResourceHandle<Mesh> fullscreen_quad;
		resources::ResourceHandle<Texture> baked_brdf;
		resources::ResourceHandle<Shader> cubemap_vertex;
		resources::ResourceHandle<Shader> equirectangular_projection_fragment;
		resources::ResourceHandle<Shader> prefiltered_specular_fragment;
		resources::ResourceHandle<Shader> diffuse_irradiance_fragment;
	};

	static void destroy(resources::ResourceManager *resource_manager, ResourceHandle<IBLTexture> handle, render::backend::Driver *driver);
	static bool process(resources::ResourceManager *resource_manager, ResourceHandle<IBLTexture> handle, const uint8_t *data, size_t size, render::backend::Driver *driver, const ImportData &import_data);
};
