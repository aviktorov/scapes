#include "RenderMaterial.h"

#include "Texture.h"

#include <cassert>
#include <iostream>

/*
 */
void resources::ResourcePipeline<RenderMaterial>::destroy(ResourceManager *resource_manager, ResourceHandle<RenderMaterial> handle, render::backend::Driver *driver)
{
	RenderMaterial *render_material = handle.get();

	driver->destroyBindSet(render_material->bindings);
	*render_material = {};
}

void resources::ResourcePipeline<RenderMaterial>::create(ResourceHandle<RenderMaterial> handle, render::backend::Driver *driver, const ImportData &import_data)
{
	RenderMaterial *render_material = handle.get();

	render_material->albedo = import_data.albedo;
	render_material->normal = import_data.normal;
	render_material->roughness = import_data.roughness;
	render_material->metalness = import_data.metalness;

	render_material->bindings = driver->createBindSet();
	driver->bindTexture(render_material->bindings, 0, render_material->albedo->gpu_data);
	driver->bindTexture(render_material->bindings, 1, render_material->normal->gpu_data);
	driver->bindTexture(render_material->bindings, 2, render_material->roughness->gpu_data);
	driver->bindTexture(render_material->bindings, 3, render_material->metalness->gpu_data);
}
