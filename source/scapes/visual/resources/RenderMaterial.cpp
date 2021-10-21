#include <scapes/visual/Resources.h>

using namespace scapes::visual;

/*
 */
void ResourcePipeline<resources::RenderMaterial>::destroy(ResourceManager *resource_manager, RenderMaterialHandle handle, render::backend::Driver *driver)
{
	resources::RenderMaterial *render_material = handle.get();

	driver->destroyBindSet(render_material->bindings);
	*render_material = {};
}
