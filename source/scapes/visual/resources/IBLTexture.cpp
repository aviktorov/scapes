#include <scapes/visual/Resources.h>
#include <RenderUtils.h>

using namespace scapes;
using namespace scapes::visual;

/*
 */
void ::ResourcePipeline<resources::IBLTexture>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	IBLTextureHandle handle,
	foundation::render::Device *device
)
{
	resources::IBLTexture *texture = handle.get();

	resource_manager->destroy(texture->prefiltered_specular_cubemap, device);
	resource_manager->destroy(texture->diffuse_irradiance_cubemap, device);

	device->destroyBindSet(texture->bindings);

	*texture = {};
}
