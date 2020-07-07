#include "Renderer.h"
#include "ApplicationResources.h"

#include <render/Mesh.h>
#include <render/SwapChain.h>

using namespace render;

/*
 */
Renderer::Renderer(backend::Driver *driver)
	: driver(driver)
	, hdriToCubeRenderer(driver)
	, diffuseIrradianceRenderer(driver)
	, environmentCubemap(driver)
	, diffuseIrradianceCubemap(driver)
	, bakedBRDF(driver)
	, bakedBRDFRenderer(driver)
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
	bakedBRDF.create2D(backend::Format::R16G16_SFLOAT, 256, 256, 1);
	environmentCubemap.createCube(backend::Format::R32G32B32A32_SFLOAT, 256, 256, 8);
	diffuseIrradianceCubemap.createCube(backend::Format::R32G32B32A32_SFLOAT, 256, 256, 1);

	bakedBRDFRenderer.init(&bakedBRDF);
	bakedBRDFRenderer.render(
		resources->getBakedBRDFVertexShader(),
		resources->getBakedBRDFFragmentShader()
	);

	hdriToCubeRenderer.init(&environmentCubemap, 0);

	cubeToPrefilteredRenderers.resize(environmentCubemap.getNumMipLevels() - 1);
	for (int mip = 0; mip < environmentCubemap.getNumMipLevels() - 1; mip++)
	{
		CubemapRenderer *mipRenderer = new CubemapRenderer(driver);
		mipRenderer->init(&environmentCubemap, mip + 1);

		cubeToPrefilteredRenderers[mip] = mipRenderer;
	}

	diffuseIrradianceRenderer.init(&diffuseIrradianceCubemap, 0);

	// Create scene descriptor set
	scene_bind_set = driver->createBindSet();

	const Texture *textures[8] =
	{
		resources->getAlbedoTexture(),
		resources->getNormalTexture(),
		resources->getAOTexture(),
		resources->getShadingTexture(),
		resources->getEmissionTexture(),
		&environmentCubemap,
		&diffuseIrradianceCubemap,
		&bakedBRDF
	};

	for (int k = 0; k < 8; k++)
		driver->bindTexture(scene_bind_set, k, textures[k]->getBackend());
}

void Renderer::shutdown()
{
	for (CubemapRenderer *renderer : cubeToPrefilteredRenderers)
	{
		renderer->shutdown();
		delete renderer;
	}
	cubeToPrefilteredRenderers.clear();

	bakedBRDFRenderer.shutdown();
	hdriToCubeRenderer.shutdown();
	diffuseIrradianceRenderer.shutdown();

	bakedBRDF.clearGPUData();
	environmentCubemap.clearGPUData();
	diffuseIrradianceCubemap.clearGPUData();

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
void Renderer::setEnvironment(const ApplicationResources *scene, const Texture *texture)
{
	hdriToCubeRenderer.render(
		scene->getCubeVertexShader(),
		scene->getHDRIToFragmentShader(),
		texture
	);

	for (uint32_t i = 0; i < cubeToPrefilteredRenderers.size(); i++)
	{
		float data[4] = {
			static_cast<float>(i) / environmentCubemap.getNumMipLevels(),
			0.0f, 0.0f, 0.0f
		};

		cubeToPrefilteredRenderers[i]->render(
			scene->getCubeVertexShader(),
			scene->getCubeToPrefilteredSpecularShader(),
			&environmentCubemap,
			i,
			static_cast<uint8_t>(sizeof(float)*4),
			reinterpret_cast<const uint8_t *>(data)
		);
	}

	diffuseIrradianceRenderer.render(
		scene->getCubeVertexShader(),
		scene->getDiffuseIrradianceFragmentShader(),
		&environmentCubemap
	);

	driver->bindTexture(scene_bind_set, 5, environmentCubemap.getBackend());
	driver->bindTexture(scene_bind_set, 6, diffuseIrradianceCubemap.getBackend());
}
