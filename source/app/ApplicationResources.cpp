#include "ApplicationResources.h"
#include "RenderUtils.h"

#include "Texture.h"
#include "Mesh.h"

#include <vector>
#include <cassert>

namespace config
{
	// Meshes
	static std::vector<const char *> meshes = {
		"assets/models/SciFiHelmet.fbx",
	};

	// Shaders
	static std::vector<const char *> shaders = {
		"assets/shaders/common/Cubemap.vert",
		"assets/shaders/common/FullscreenQuad.vert",
		"assets/shaders/utils/EquirectangularProjection.frag",
		"assets/shaders/utils/PrefilteredSpecularCubemap.frag",
		"assets/shaders/utils/DiffuseIrradianceCubemap.frag",
		"assets/shaders/utils/BakedBRDF.frag",
		"assets/shaders/deferred/SkylightDeferred.frag",
		"assets/shaders/deferred/GBuffer.vert",
		"assets/shaders/deferred/GBuffer.frag",
		"assets/shaders/deferred/SSAO.frag",
		"assets/shaders/deferred/SSAOBlur.frag",
		"assets/shaders/deferred/SSRTrace.frag",
		"assets/shaders/deferred/SSRResolve.frag",
		"assets/shaders/deferred/Composite.frag",
		"assets/shaders/render/TemporalFilter.frag",
		"assets/shaders/render/Tonemapping.frag",
		"assets/shaders/render/Final.frag",
	};

	static std::vector<render::backend::ShaderType> shaderTypes = {
		render::backend::ShaderType::VERTEX,
		render::backend::ShaderType::VERTEX,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::VERTEX,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
		render::backend::ShaderType::FRAGMENT,
	};

	// Textures
	static std::vector<const char *> textures = {
		"assets/textures/SciFiHelmet_BaseColor.png",
		"assets/textures/SciFiHelmet_Normal.png",
		"assets/textures/SciFiHelmet_AmbientOcclusion.png",
		"assets/textures/SciFiHelmet_MetallicRoughness.png",
		"assets/textures/Default_emissive.jpg",
	};

	static std::vector<const char *> hdrTextures = {
		"assets/textures/environment/arctic.hdr",
		"assets/textures/environment/umbrellas.hdr",
		"assets/textures/environment/shanghai_bund_4k.hdr",
	};

	static const char *blueNoise = "assets/textures/blue_noise.png";
}

static render::backend::Texture *generateTexture(render::backend::Driver *driver, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t pixels[16] = {
		r, g, b, 255,
		r, g, b, 255,
		r, g, b, 255,
		r, g, b, 255,
	};

	return driver->createTexture2D(2, 2, 1, render::backend::Format::R8G8B8A8_UNORM, pixels);
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
		512, 512, 1,
		resources.getShader(config::Shaders::FullscreenQuadVertex),
		resources.getShader(config::Shaders::BakedBRDFFragment)
	);

	baked_brdf->setSamplerWrapMode(render::backend::SamplerWrapMode::CLAMP_TO_EDGE);

	environment_cubemaps.resize(config::hdrTextures.size());
	irradiance_cubemaps.resize(config::hdrTextures.size());

	for (int i = 0; i < config::hdrTextures.size(); ++i)
	{
		environment_cubemaps[i] = RenderUtils::hdriToCube(
			driver,
			render::backend::Format::R32G32B32A32_SFLOAT,
			128,
			resources.getTexture(config::Textures::EnvironmentBase + i),
			resources.getShader(config::Shaders::CubemapVertex),
			resources.getShader(config::Shaders::EquirectangularProjectionFragment),
			resources.getShader(config::Shaders::PrefilteredSpecularCubemapFragment)
		);

		irradiance_cubemaps[i] = RenderUtils::createTextureCube(
			driver,
			render::backend::Format::R32G32B32A32_SFLOAT,
			128,
			1,
			resources.getShader(config::Shaders::CubemapVertex),
			resources.getShader(config::Shaders::DiffuseIrradianceCubemapFragment),
			environment_cubemaps[i]
		);
	}

	blue_noise = new Texture(driver);
	blue_noise->import(config::blueNoise);

	fullscreen_quad = new Mesh(driver);
	fullscreen_quad->createQuad(2.0f);

	default_albedo = generateTexture(driver, 127, 127, 127);
	default_normal = generateTexture(driver, 127, 127, 255);
	default_roughness = generateTexture(driver, 255, 255, 255);
	default_metalness = generateTexture(driver, 0, 0, 0);
}

void ApplicationResources::shutdown()
{
	resources.clear();

	delete baked_brdf;
	baked_brdf = nullptr;

	for (int i = 0; i < environment_cubemaps.size(); ++i)
		delete environment_cubemaps[i];
	
	for (int i = 0; i < irradiance_cubemaps.size(); ++i)
		delete irradiance_cubemaps[i];

	environment_cubemaps.clear();
	irradiance_cubemaps.clear();

	delete blue_noise;
	blue_noise = nullptr;

	delete fullscreen_quad;
	fullscreen_quad = nullptr;

	driver->destroyTexture(default_albedo);
	driver->destroyTexture(default_normal);
	driver->destroyTexture(default_roughness);
	driver->destroyTexture(default_metalness);

	default_albedo = nullptr;
	default_normal = nullptr;
	default_roughness = nullptr;
	default_metalness = nullptr;
}

void ApplicationResources::reloadShaders()
{
	for (int i = 0; i < config::shaders.size(); ++i)
		resources.reloadShader(i);
}
