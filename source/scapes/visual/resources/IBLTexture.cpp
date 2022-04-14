#include <scapes/visual/IBLTexture.h>
#include <scapes/visual/hardware/Device.h>

using namespace scapes;
using namespace scapes::visual;

/*
 */
size_t ResourceTraits<IBLTexture>::size()
{
	return sizeof(IBLTexture);
}

void ResourceTraits<IBLTexture>::create(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	scapes::visual::hardware::Device *device
)
{
	IBLTexture *texture = reinterpret_cast<IBLTexture *>(memory);

	*texture = {};
	texture->device = device;
}

void ResourceTraits<IBLTexture>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	IBLTexture *texture = reinterpret_cast<IBLTexture *>(memory);
	scapes::visual::hardware::Device *device = texture->device;

	assert(device);

	device->destroyTexture(texture->prefiltered_specular_cubemap);
	device->destroyTexture(texture->diffuse_irradiance_cubemap);
	device->destroyBindSet(texture->bindings);

	*texture = {};
}

foundation::resources::hash_t ResourceTraits<IBLTexture>::fetchHash(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	return 0;
}

bool ResourceTraits<IBLTexture>::reload(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	return false;
}
