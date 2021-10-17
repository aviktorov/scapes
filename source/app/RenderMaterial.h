#pragma once

#include <render/backend/Driver.h>
#include <common/ResourceManager.h>

struct Mesh;
struct Shader;
struct Texture;

/*
 */
struct RenderMaterial
{
	resources::ResourceHandle<Texture> albedo;
	resources::ResourceHandle<Texture> normal;
	resources::ResourceHandle<Texture> roughness;
	resources::ResourceHandle<Texture> metalness;
	::render::backend::BindSet *bindings {nullptr};
};

template <>
struct TypeTraits<RenderMaterial>
{
	static constexpr const char *name = "RenderMaterial";
};

template <>
struct resources::ResourcePipeline<RenderMaterial>
{
	struct ImportData
	{
		resources::ResourceHandle<Texture> albedo;
		resources::ResourceHandle<Texture> normal;
		resources::ResourceHandle<Texture> roughness;
		resources::ResourceHandle<Texture> metalness;
	};

	static void destroy(resources::ResourceManager *resource_manager, ResourceHandle<RenderMaterial> handle, render::backend::Driver *driver);

	// TODO: move from resource pipeline
	static void create(ResourceHandle<RenderMaterial> handle, render::backend::Driver *driver, const ImportData &import_data);
};
