#include "ApplicationResources.h"
#include "RenderUtils.h"

#include <render/Texture.h>

#include <vector>
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
		"shaders/deferred/skylight_deferred.vert",
		"shaders/deferred/skylight_deferred.frag",
		"shaders/deferred/gbuffer.vert",
		"shaders/deferred/gbuffer.frag",
		"shaders/deferred/ssao.vert",
		"shaders/deferred/ssao.frag",
		"shaders/deferred/composite.vert",
		"shaders/deferred/composite.frag",
		"shaders/deferred/final.vert",
		"shaders/deferred/final.frag",
	};

	static std::vector<render::backend::ShaderType> shaderTypes = {
		render::backend::ShaderType::VERTEX,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::VERTEX,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::VERTEX,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::VERTEX,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::VERTEX,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::VERTEX,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::VERTEX,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::VERTEX,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::VERTEX,
		render::backend::ShaderType::FRAGMENT,
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
const char *ApplicationResources::getHDRTexturePath(int index) const
{
	assert(index >= 0 && index < config::hdrTextures.size());
	return config::hdrTextures[index];
}

size_t ApplicationResources::getNumHDRTextures() const
{
	return config::hdrTextures.size();
}

/*
 */
ApplicationResources::~ApplicationResources()
{
	shutdown();
}

/*
 */
void ApplicationResources::init()
{
	for (int i = 0; i < config::meshes.size(); ++i)
		resources.loadMesh(i, config::meshes[i]);

	resources.createCubeMesh(config::Meshes::Skybox, 10000.0f);

	for (int i = 0; i < config::shaders.size(); ++i)
		resources.loadShader(i, config::shaderTypes[i], config::shaders[i]);

	for (int i = 0; i < config::textures.size(); ++i)
		resources.loadTexture(i, config::textures[i]);

	for (int i = 0; i < config::hdrTextures.size(); ++i)
		resources.loadTexture(config::Textures::EnvironmentBase + i, config::hdrTextures[i]);

	baked_brdf = RenderUtils::createTexture2D(
		driver,
		render::backend::Format::R16G16_SFLOAT,
		256, 256, 1,
		resources.getShader(config::Shaders::BakedBRDFVertex),
		resources.getShader(config::Shaders::BakedBRDFFragment)
	);

	environment_cubemaps.resize(config::hdrTextures.size());
	irradiance_cubemaps.resize(config::hdrTextures.size());

	for (int i = 0; i < config::hdrTextures.size(); ++i)
	{
		environment_cubemaps[i] = RenderUtils::hdriToCube(
			driver,
			render::backend::Format::R32G32B32A32_SFLOAT,
			256,
			resources.getTexture(config::Textures::EnvironmentBase + i),
			resources.getShader(config::Shaders::CubeVertex),
			resources.getShader(config::Shaders::HDRIToCubeFragment),
			resources.getShader(config::Shaders::CubeToPrefilteredSpecular)
		);

		irradiance_cubemaps[i] = RenderUtils::createTextureCube(
			driver,
			render::backend::Format::R32G32B32A32_SFLOAT,
			256,
			1,
			resources.getShader(config::Shaders::CubeVertex),
			resources.getShader(config::Shaders::DiffuseIrradianceFragment),
			environment_cubemaps[i]
		);
	}
}

void ApplicationResources::shutdown()
{
	for (int i = 0; i < config::meshes.size(); ++i)
		resources.unloadMesh(i);

	resources.unloadMesh(config::Meshes::Skybox);

	for (int i = 0; i < config::shaders.size(); ++i)
		resources.unloadShader(i);

	for (int i = 0; i < config::textures.size(); ++i)
		resources.unloadTexture(i);

	for (int i = 0; i < config::hdrTextures.size(); ++i)
		resources.unloadTexture(config::Textures::EnvironmentBase + i);

	delete baked_brdf;
	baked_brdf = nullptr;

	for (int i = 0; i < environment_cubemaps.size(); ++i)
		delete environment_cubemaps[i];
	
	for (int i = 0; i < irradiance_cubemaps.size(); ++i)
		delete irradiance_cubemaps[i];

	environment_cubemaps.clear();
	irradiance_cubemaps.clear();
}

void ApplicationResources::reloadShaders()
{
	for (int i = 0; i < config::shaders.size(); ++i)
		resources.reloadShader(i);
}
