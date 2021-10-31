#include <scapes/visual/Resources.h>

using namespace scapes;
using namespace scapes::visual;

/*
 */
void ResourcePipeline<resources::RenderMaterial>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	RenderMaterialHandle handle,
	foundation::render::Device *device
)
{
	resources::RenderMaterial *render_material = handle.get();

	device->destroyBindSet(render_material->bindings);
	*render_material = {};
}
