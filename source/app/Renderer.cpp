#include "Renderer.h"
#include "ApplicationResources.h"

#include <render/Mesh.h>
#include <render/SwapChain.h>

using namespace render;

/*
 */
Renderer::Renderer(backend::Driver *driver)
	: driver(driver)
{
}

Renderer::~Renderer()
{
	shutdown();
}

/*
 */
void Renderer::init(const ApplicationResources *resources)
{
	// Create scene descriptor set
	scene_bind_set = driver->createBindSet();

	const Texture *textures[8] =
	{
		resources->getAlbedoTexture(),
		resources->getNormalTexture(),
		resources->getAOTexture(),
		resources->getShadingTexture(),
		resources->getEmissionTexture(),
		resources->getHDREnvironmentCubemap(0),
		resources->getHDRIrradianceCubemap(0),
		resources->getBakedBRDFTexture()
	};

	for (int i = 0; i < 8; i++)
		driver->bindTexture(scene_bind_set, i, textures[i]->getBackend());
}

void Renderer::shutdown()
{
	driver->destroyBindSet(scene_bind_set);
	scene_bind_set = nullptr;
}

void Renderer::render(const ApplicationResources *scene, const RenderFrame &frame)
{
	const Shader *pbrVertexShader = scene->getPBRVertexShader();
	const Shader *pbrFragmentShader = scene->getPBRFragmentShader();
	const Shader *skyboxVertexShader = scene->getSkyboxVertexShader();
	const Shader *skyboxFragmentShader = scene->getSkyboxFragmentShader();

	const Mesh *skybox = scene->getSkybox();
	const Mesh *mesh = scene->getMesh();

	driver->clearPushConstants();
	driver->clearBindSets();
	driver->pushBindSet(frame.bind_set);
	driver->pushBindSet(scene_bind_set);

	driver->clearShaders();

	driver->setShader(backend::ShaderType::VERTEX, skyboxVertexShader->getBackend());
	driver->setShader(backend::ShaderType::FRAGMENT, skyboxFragmentShader->getBackend());
	driver->drawIndexedPrimitive(frame.command_buffer, skybox->getRenderPrimitive());

	driver->setShader(backend::ShaderType::VERTEX, pbrVertexShader->getBackend());
	driver->setShader(backend::ShaderType::FRAGMENT, pbrFragmentShader->getBackend());
	driver->drawIndexedPrimitive(frame.command_buffer, mesh->getRenderPrimitive());
}

/*
 */
void Renderer::setEnvironment(const ApplicationResources *resources, int index)
{
	const Texture *environment_cubemap = resources->getHDREnvironmentCubemap(index);
	const Texture *irradiance_cubemap = resources->getHDRIrradianceCubemap(index);

	driver->bindTexture(scene_bind_set, 5, environment_cubemap->getBackend());
	driver->bindTexture(scene_bind_set, 6, irradiance_cubemap->getBackend());
}
