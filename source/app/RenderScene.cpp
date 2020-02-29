#include "RenderScene.h"
#include "VulkanContext.h"
#include "VulkanShader.h"

#include <cassert>

namespace config
{
	// Meshes
	static std::vector<const char *>meshes = {
		"models/SciFiHelmet.fbx",
	};

	// Shaders
	static std::vector<const char *> shaders = {
		"shaders/pbr.vert",
		"shaders/pbr.frag",
		"shaders/skybox.vert",
		"shaders/skybox.frag",
		"shaders/commonCube.vert",
		"shaders/hdriToCube.frag",
		"shaders/cubeToPrefilteredSpecular.frag",
		"shaders/diffuseIrradiance.frag",
		"shaders/bakedBRDF.vert",
		"shaders/bakedBRDF.frag",
	};

	// Textures
	static std::vector<const char *> textures = {
		"textures/SciFiHelmet_BaseColor.png",
		"textures/SciFiHelmet_Normal.png",
		"textures/SciFiHelmet_AmbientOcclusion.png",
		"textures/SciFiHelmet_MetallicRoughness.png",
		"textures/Default_emissive.jpg",
	};

	static std::vector<const char *> hdrTextures = {
		"textures/environment/arctic.hdr",
		"textures/environment/umbrellas.hdr",
		"textures/environment/shanghai_bund_4k.hdr",
	};
}

/*
 */
RenderScene::RenderScene(const VulkanContext *context)
	: resources(context)
{ }

/*
 */
const char *RenderScene::getHDRTexturePath(int index) const
{
	assert(index >= 0 && index < config::hdrTextures.size());
	return config::hdrTextures[index];
}

size_t RenderScene::getNumHDRTextures() const
{
	return config::hdrTextures.size();
}

/*
 */
void RenderScene::init()
{
	for (int i = 0; i < config::meshes.size(); i++)
		resources.loadMesh(i, config::meshes[i]);

	resources.createCubeMesh(config::Meshes::Skybox, 10000.0f);

	for (int i = 0; i < config::shaders.size(); i++)
		resources.loadShader(i, config::shaders[i]);

	for (int i = 0; i < config::textures.size(); i++)
		resources.loadTexture(i, config::textures[i]);

	for (int i = 0; i < config::hdrTextures.size(); i++)
		resources.loadTexture(config::Textures::EnvironmentBase + i, config::hdrTextures[i]);
}

void RenderScene::shutdown()
{
	for (int i = 0; i < config::meshes.size(); i++)
		resources.unloadMesh(i);

	resources.unloadMesh(config::Meshes::Skybox);

	for (int i = 0; i < config::shaders.size(); i++)
		resources.unloadShader(i);

	for (int i = 0; i < config::textures.size(); i++)
		resources.unloadTexture(i);

	for (int i = 0; i < config::hdrTextures.size(); i++)
		resources.unloadTexture(config::Textures::EnvironmentBase + i);
}

void RenderScene::reloadShaders()
{
	for (int i = 0; i < config::shaders.size(); i++)
		resources.reloadShader(i);
}
