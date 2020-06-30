#include "VulkanRenderer.h"
#include "VulkanMesh.h"
#include "VulkanSwapChain.h"

#include "RenderScene.h"

using namespace render::backend;

/*
 */
VulkanRenderer::VulkanRenderer(render::backend::Driver *driver)
	: driver(driver)
	, hdriToCubeRenderer(driver)
	, diffuseIrradianceRenderer(driver)
	, environmentCubemap(driver)
	, diffuseIrradianceCubemap(driver)
	, bakedBRDF(driver)
	, bakedBRDFRenderer(driver)
{
}

VulkanRenderer::~VulkanRenderer()
{
	shutdown();
}

/*
 */
void VulkanRenderer::init(const RenderScene *scene)
{
	bakedBRDF.create2D(render::backend::Format::R16G16_SFLOAT, 256, 256, 1);
	environmentCubemap.createCube(render::backend::Format::R32G32B32A32_SFLOAT, 256, 256, 8);
	diffuseIrradianceCubemap.createCube(render::backend::Format::R32G32B32A32_SFLOAT, 256, 256, 1);

	bakedBRDFRenderer.init(
		*scene->getBakedBRDFVertexShader(),
		*scene->getBakedBRDFFragmentShader(),
		bakedBRDF
	);

	bakedBRDFRenderer.render();

	hdriToCubeRenderer.init(
		*scene->getCubeVertexShader(),
		*scene->getHDRIToFragmentShader(),
		environmentCubemap,
		0
	);

	cubeToPrefilteredRenderers.resize(environmentCubemap.getNumMipLevels() - 1);
	for (int mip = 0; mip < environmentCubemap.getNumMipLevels() - 1; mip++)
	{
		VulkanCubemapRenderer *mipRenderer = new VulkanCubemapRenderer(driver);
		mipRenderer->init(
			*scene->getCubeVertexShader(),
			*scene->getCubeToPrefilteredSpecularShader(),
			environmentCubemap,
			mip + 1,
			sizeof(float) * 4
		);

		cubeToPrefilteredRenderers[mip] = mipRenderer;
	}

	diffuseIrradianceRenderer.init(
		*scene->getCubeVertexShader(),
		*scene->getDiffuseIrradianceFragmentShader(),
		diffuseIrradianceCubemap,
		0
	);

	// Create scene descriptor set
	scene_bind_set = driver->createBindSet();

	std::array<const VulkanTexture *, 8> textures =
	{
		scene->getAlbedoTexture(),
		scene->getNormalTexture(),
		scene->getAOTexture(),
		scene->getShadingTexture(),
		scene->getEmissionTexture(),
		&environmentCubemap,
		&diffuseIrradianceCubemap,
		&bakedBRDF
	};

	for (int k = 0; k < textures.size(); k++)
		driver->bindTexture(scene_bind_set, k, textures[k]->getBackend());
}

void VulkanRenderer::shutdown()
{
	for (VulkanCubemapRenderer *renderer : cubeToPrefilteredRenderers)
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

void VulkanRenderer::render(const RenderScene *scene, const VulkanRenderFrame &frame)
{
	const VulkanShader *pbrVertexShader = scene->getPBRVertexShader();
	const VulkanShader *pbrFragmentShader = scene->getPBRFragmentShader();
	const VulkanShader *skyboxVertexShader = scene->getSkyboxVertexShader();
	const VulkanShader *skyboxFragmentShader = scene->getSkyboxFragmentShader();

	const VulkanMesh *skybox = scene->getSkybox();
	const VulkanMesh *mesh = scene->getMesh();

	driver->clearBindSets();
	driver->pushBindSet(frame.bind_set);
	driver->pushBindSet(scene_bind_set);

	driver->clearShaders();

	driver->setShader(ShaderType::VERTEX, skyboxVertexShader->getBackend());
	driver->setShader(ShaderType::FRAGMENT, skyboxFragmentShader->getBackend());
	driver->drawIndexedPrimitive(frame.command_buffer, skybox->getRenderPrimitive());

	driver->setShader(ShaderType::VERTEX, pbrVertexShader->getBackend());
	driver->setShader(ShaderType::FRAGMENT, pbrFragmentShader->getBackend());
	driver->drawIndexedPrimitive(frame.command_buffer, mesh->getRenderPrimitive());
}

/*
 */
void VulkanRenderer::reload(const RenderScene *scene)
{
	shutdown();
	init(scene);
}

void VulkanRenderer::setEnvironment(const VulkanTexture *texture)
{
	hdriToCubeRenderer.render(*texture);

	for (uint32_t i = 0; i < cubeToPrefilteredRenderers.size(); i++)
	{
		float data[4] = {
			static_cast<float>(i) / environmentCubemap.getNumMipLevels(),
			0.0f, 0.0f, 0.0f
		};

		cubeToPrefilteredRenderers[i]->render(environmentCubemap, data, i);
	}

	diffuseIrradianceRenderer.render(environmentCubemap);

	std::array<const VulkanTexture *, 2> textures =
	{
		&environmentCubemap,
		&diffuseIrradianceCubemap,
	};

	driver->bindTexture(scene_bind_set, 5, environmentCubemap.getBackend());
	driver->bindTexture(scene_bind_set, 6, diffuseIrradianceCubemap.getBackend());
}
