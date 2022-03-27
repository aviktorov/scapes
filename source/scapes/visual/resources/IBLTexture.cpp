#include <scapes/visual/Resources.h>

using namespace scapes;
using namespace scapes::visual;

/*
 */
size_t ResourceTraits<resources::IBLTexture>::size()
{
	return sizeof(resources::IBLTexture);
}

void ResourceTraits<resources::IBLTexture>::create(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	resources::IBLTexture *texture = reinterpret_cast<resources::IBLTexture *>(memory);

	*texture = {};
}

void ResourceTraits<resources::IBLTexture>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	resources::IBLTexture *texture = reinterpret_cast<resources::IBLTexture *>(memory);

	// resource_manager->destroy(texture->prefiltered_specular_cubemap, device);
	// resource_manager->destroy(texture->diffuse_irradiance_cubemap, device);

	// device->destroyBindSet(texture->bindings);

	*texture = {};
}
