#include "SkyLight.h"

#include "Mesh.h"
#include "Texture.h"

/*
 */
Light::Light(render::backend::Driver *driver, const Shader *vertex, const Shader *fragment)
	: driver(driver), vertex_shader(vertex), fragment_shader(fragment)
{
	bind_set = driver->createBindSet();
}

Light::~Light()
{
	driver->destroyBindSet(bind_set);
	bind_set = nullptr;

	mesh = nullptr;
	vertex_shader = nullptr;
	fragment_shader = nullptr;
}

/*
 */
SkyLight::SkyLight(render::backend::Driver *driver, const Shader *vertex, const Shader *fragment)
	: Light(driver, vertex, fragment)
{
	mesh = new Mesh(driver);
	mesh->createQuad(2.0f);
}

SkyLight::~SkyLight()
{
	delete mesh;
}

void SkyLight::setBakedBRDFTexture(const Texture *brdf_texture)
{
	driver->bindTexture(bind_set, 0, brdf_texture->getBackend());
}

void SkyLight::setEnvironmentCubemap(const Texture *environment_cubemap)
{
	driver->bindTexture(bind_set, 1, environment_cubemap->getBackend());
}

void SkyLight::setIrradianceCubemap(const Texture *irradiance_cubemap)
{
	driver->bindTexture(bind_set, 2, irradiance_cubemap->getBackend());
}
