#include <scapes/visual/Resources.h>

using namespace scapes;
using namespace scapes::visual;

/*
 */
size_t ResourceTraits<resources::RenderMaterial>::size()
{
	return sizeof(resources::RenderMaterial);
}

void ResourceTraits<resources::RenderMaterial>::create(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	foundation::render::Device *device
)
{
	resources::RenderMaterial *render_material = reinterpret_cast<resources::RenderMaterial *>(memory);

	*render_material = {};
	render_material->device = device;
}

void ResourceTraits<resources::RenderMaterial>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	resources::RenderMaterial *render_material = reinterpret_cast<resources::RenderMaterial *>(memory);
	foundation::render::Device *device = render_material->device;

	assert(device);

	device->destroyBindSet(render_material->bindings);

	*render_material = {};
}
