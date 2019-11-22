#include "RenderScene.h"
#include "VulkanShader.h"

namespace config
{
	// Meshes
	static std::vector<const char *>meshes = {
		"models/SciFiHelmet.gltf",
	};

	// Shaders
	static std::vector<const char *> shaders = {
		"shaders/pbr.vert",
		"shaders/pbr.frag",
		"shaders/skybox.vert",
		"shaders/skybox.frag",
		"shaders/commonCube.vert",
		"shaders/hdriToCube.frag",
		"shaders/diffuseIrradiance.frag",
	};

	static std::vector<VulkanShaderKind> shaderKinds = {
		VulkanShaderKind::Vertex,
		VulkanShaderKind::Fragment,
		VulkanShaderKind::Vertex,
		VulkanShaderKind::Fragment,
		VulkanShaderKind::Vertex,
		VulkanShaderKind::Fragment,
		VulkanShaderKind::Fragment,
	};

	// Textures
	static std::vector<const char *> textures = {
		"textures/SciFiHelmet_BaseColor.png",
		"textures/SciFiHelmet_Normal.png",
		"textures/SciFiHelmet_AmbientOcclusion.png",
		"textures/SciFiHelmet_MetallicRoughness.png",
		"textures/Default_emissive.jpg",
	};

	static const char *hdrTexture = "textures/Default_environment.hdr";
}

/*
 */
RenderScene::RenderScene(const VulkanRendererContext &context)
	: resources(context)
{ }

/*
 */
void RenderScene::init()
{
	for (int i = 0; i < config::meshes.size(); i++)
		resources.loadMesh(i, config::meshes[i]);

	resources.createCubeMesh(config::Meshes::Skybox, 1000.0f);

	for (int i = 0; i < config::shaders.size(); i++)
		resources.loadShader(i, config::shaderKinds[i], config::shaders[i]);

	for (int i = 0; i < config::textures.size(); i++)
		resources.loadTexture(i, config::textures[i]);

	resources.loadHDRTexture(config::Textures::Environment, config::hdrTexture);
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

	resources.unloadTexture(config::Textures::Environment);
}
