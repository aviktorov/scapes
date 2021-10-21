#include <scapes/visual/Resources.h>
#include <RenderUtils.h>

using namespace scapes::visual;

/*
 */
void ::ResourcePipeline<resources::IBLTexture>::destroy(ResourceManager *resource_manager, IBLTextureHandle handle, render::backend::Driver *driver)
{
	resources::IBLTexture *texture = handle.get();

	resource_manager->destroy(texture->prefiltered_specular_cubemap, driver);
	resource_manager->destroy(texture->diffuse_irradiance_cubemap, driver);

	driver->destroyBindSet(texture->bindings);

	*texture = {};
}
