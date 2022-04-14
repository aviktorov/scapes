#include <scapes/visual/RenderMaterial.h>
#include <scapes/visual/Texture.h>
#include <scapes/visual/hardware/Device.h>

#include <HashUtils.h>

using namespace scapes;
using namespace scapes::visual;

/*
 */
size_t ResourceTraits<RenderMaterial>::size()
{
	return sizeof(RenderMaterial);
}

void ResourceTraits<RenderMaterial>::create(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	TextureHandle albedo,
	TextureHandle normal,
	TextureHandle roughness,
	TextureHandle metalness,
	scapes::visual::hardware::Device *device
)
{
	RenderMaterial *render_material = reinterpret_cast<RenderMaterial *>(memory);

	*render_material = {};
	render_material->device = device;

	render_material->bindings = device->createBindSet();
	render_material->albedo = albedo;
	render_material->normal = normal;
	render_material->roughness = roughness;
	render_material->metalness = metalness;

	device->bindTexture(render_material->bindings, 0, render_material->albedo->gpu_data);
	device->bindTexture(render_material->bindings, 1, render_material->normal->gpu_data);
	device->bindTexture(render_material->bindings, 2, render_material->roughness->gpu_data);
	device->bindTexture(render_material->bindings, 3, render_material->metalness->gpu_data);
}

void ResourceTraits<RenderMaterial>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	RenderMaterial *render_material = reinterpret_cast<RenderMaterial *>(memory);
	scapes::visual::hardware::Device *device = render_material->device;

	assert(device);

	device->destroyBindSet(render_material->bindings);

	*render_material = {};
}

foundation::resources::hash_t ResourceTraits<RenderMaterial>::fetchHash(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	return 0;
}

bool ResourceTraits<RenderMaterial>::reload(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	return false;
}
