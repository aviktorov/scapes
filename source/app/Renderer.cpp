#include "Renderer.h"
#include "ApplicationResources.h"
#include "SkyLight.h"

#include <render/backend/driver.h>
#include <render/Mesh.h>
#include <render/Texture.h>
#include <render/Shader.h>
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

	const Texture *textures[5] =
	{
		resources->getAlbedoTexture(),
		resources->getNormalTexture(),
		resources->getAOTexture(),
		resources->getShadingTexture(),
		resources->getEmissionTexture(),
	};

	for (int i = 0; i < 5; i++)
		driver->bindTexture(scene_bind_set, i, textures[i]->getBackend());
}

void Renderer::shutdown()
{
	driver->destroyBindSet(scene_bind_set);
	scene_bind_set = nullptr;
}

void Renderer::render(const ApplicationResources *scene, const SkyLight *sky_light, const RenderFrame &frame)
{
	const Shader *pbr_vertex_shader = scene->getPBRVertexShader();
	const Shader *pbr_fragment_shader = scene->getPBRFragmentShader();
	const Shader *skybox_vertex_shader = scene->getSkyboxVertexShader();
	const Shader *skybox_fragment_shader = scene->getSkyboxFragmentShader();

	const Mesh *skybox = scene->getSkybox();
	const Mesh *mesh = scene->getMesh();

	driver->setBlending(false);
	driver->setCullMode(backend::CullMode::BACK);
	driver->setDepthWrite(true);
	driver->setDepthTest(true);

	driver->clearPushConstants();
	driver->clearBindSets();
	driver->pushBindSet(frame.bindings);
	driver->pushBindSet(scene_bind_set);
	driver->pushBindSet(sky_light->getBindSet());

	driver->clearShaders();

	driver->setShader(backend::ShaderType::VERTEX, skybox_vertex_shader->getBackend());
	driver->setShader(backend::ShaderType::FRAGMENT, skybox_fragment_shader->getBackend());
	driver->drawIndexedPrimitive(frame.command_buffer, skybox->getRenderPrimitive());

	driver->setShader(backend::ShaderType::VERTEX, pbr_vertex_shader->getBackend());
	driver->setShader(backend::ShaderType::FRAGMENT, pbr_fragment_shader->getBackend());
	driver->drawIndexedPrimitive(frame.command_buffer, mesh->getRenderPrimitive());
}
