#include "SkyLight.h"

#include "Mesh.h"
#include "Texture.h"

/*
 */
Light::Light(render::backend::Driver *driver, resources::ResourceHandle<Mesh> mesh, const Shader *vertex, const Shader *fragment)
	: driver(driver), mesh(mesh), vertex_shader(vertex), fragment_shader(fragment)
{
	bind_set = driver->createBindSet();
}

Light::~Light()
{
	driver->destroyBindSet(bind_set);
	bind_set = nullptr;

	vertex_shader = nullptr;
	fragment_shader = nullptr;
}

/*
 */
SkyLight::SkyLight(render::backend::Driver *driver, resources::ResourceHandle<Mesh> mesh, const Shader *vertex, const Shader *fragment)
	: Light(driver, mesh, vertex, fragment)
{
}

SkyLight::~SkyLight()
{
}

void SkyLight::setBakedBRDFTexture(const Texture *brdf_texture)
{
	driver->bindTexture(bind_set, 0, brdf_texture->gpu_data);
}

void SkyLight::setEnvironmentCubemap(const Texture *environment_cubemap)
{
	driver->bindTexture(bind_set, 1, environment_cubemap->gpu_data);
}

void SkyLight::setIrradianceCubemap(const Texture *irradiance_cubemap)
{
	driver->bindTexture(bind_set, 2, irradiance_cubemap->gpu_data);
}
