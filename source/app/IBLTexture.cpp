#include "IBLTexture.h"

#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "RenderUtils.h"

#include <cassert>
#include <iostream>

/*
 */
void resources::ResourcePipeline<IBLTexture>::destroy(ResourceManager *resource_manager, ResourceHandle<IBLTexture> handle, render::backend::Driver *driver)
{
	IBLTexture *texture = handle.get();

	resource_manager->destroy(texture->prefiltered_specular_cubemap, driver);
	resource_manager->destroy(texture->diffuse_irradiance_cubemap, driver);

	driver->destroyBindSet(texture->bindings);

	*texture = {};
}

bool resources::ResourcePipeline<IBLTexture>::process(ResourceManager *resource_manager, ResourceHandle<IBLTexture> handle, const uint8_t *data, size_t size, render::backend::Driver *driver, const ImportData &import_data)
{
	IBLTexture *texture = handle.get();

	resources::ResourceHandle<Texture> equirectangular_texture = resource_manager->importFromMemory<Texture>(data, size, driver);
	if (equirectangular_texture.get() == nullptr)
		return false;

	texture->prefiltered_specular_cubemap = RenderUtils::hdriToCube(
		resource_manager,
		driver,
		import_data.format,
		import_data.cubemap_size,
		equirectangular_texture,
		import_data.fullscreen_quad,
		import_data.cubemap_vertex,
		import_data.equirectangular_projection_fragment,
		import_data.prefiltered_specular_fragment
	);

	texture->diffuse_irradiance_cubemap = RenderUtils::createTextureCube(
		resource_manager,
		driver,
		import_data.format,
		import_data.cubemap_size,
		1,
		import_data.fullscreen_quad,
		import_data.cubemap_vertex,
		import_data.diffuse_irradiance_fragment,
		texture->prefiltered_specular_cubemap
	);

	resource_manager->destroy(equirectangular_texture, driver);

	texture->baked_brdf = import_data.baked_brdf;

	texture->bindings = driver->createBindSet();
	driver->bindTexture(texture->bindings, 0, texture->baked_brdf->gpu_data);
	driver->bindTexture(texture->bindings, 1, texture->prefiltered_specular_cubemap->gpu_data);
	driver->bindTexture(texture->bindings, 2, texture->diffuse_irradiance_cubemap->gpu_data);

	return true;
}
