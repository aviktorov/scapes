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
	void *memory,
	foundation::render::Device *device
)
{
	resources::IBLTexture *texture = reinterpret_cast<resources::IBLTexture *>(memory);

	*texture = {};
	texture->device = device;
}

void ResourceTraits<resources::IBLTexture>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	resources::IBLTexture *texture = reinterpret_cast<resources::IBLTexture *>(memory);
	foundation::render::Device *device = texture->device;

	assert(device);

	device->destroyTexture(texture->prefiltered_specular_cubemap);
	device->destroyTexture(texture->diffuse_irradiance_cubemap);
	device->destroyBindSet(texture->bindings);

	*texture = {};
}

foundation::resources::hash_t ResourceTraits<resources::IBLTexture>::fetchHash(
	foundation::resources::ResourceManager *resource_manager,
	const foundation::io::URI &uri
)
{
	return 0;
}

bool ResourceTraits<resources::IBLTexture>::reload(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	const foundation::io::URI &uri
)
{
	return false;
}
