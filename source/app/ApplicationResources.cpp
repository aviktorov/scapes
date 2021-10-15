#include "ApplicationResources.h"
#include "RenderUtils.h"

#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"

#include <cassert>
#include <iostream>

namespace config
{
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
	for (int i = 0; i < config::shaders.size(); ++i)
		loadShader(i, config::shaderTypes[i], config::shaders[i]);

	for (int i = 0; i < config::hdrTextures.size(); ++i)
		hdr_cubemaps.push_back(loadTexture(config::hdrTextures[i]));

	fullscreen_quad = resource_manager->create<Mesh>();
	resources::ResourcePipeline<Mesh>::createQuad(fullscreen_quad, driver, 2.0f);

	skybox = resource_manager->create<Mesh>();
	resources::ResourcePipeline<Mesh>::createSkybox(skybox, driver, 10000.0f);

	baked_brdf = RenderUtils::createTexture2D(
		resource_manager,
		driver,
		render::backend::Format::R16G16_SFLOAT,
		512, 512, 1,
		fullscreen_quad,
		getShader(config::Shaders::FullscreenQuadVertex),
		getShader(config::Shaders::BakedBRDFFragment)
	);

	driver->setTextureSamplerWrapMode(baked_brdf.get()->gpu_data, render::backend::SamplerWrapMode::CLAMP_TO_EDGE);

	environment_cubemaps.resize(config::hdrTextures.size());
	irradiance_cubemaps.resize(config::hdrTextures.size());

	for (int i = 0; i < config::hdrTextures.size(); ++i)
	{
		environment_cubemaps[i] = RenderUtils::hdriToCube(
			resource_manager,
			driver,
			render::backend::Format::R32G32B32A32_SFLOAT,
			128,
			hdr_cubemaps[i],
			fullscreen_quad,
			getShader(config::Shaders::CubemapVertex),
			getShader(config::Shaders::EquirectangularProjectionFragment),
			getShader(config::Shaders::PrefilteredSpecularCubemapFragment)
		);

		irradiance_cubemaps[i] = RenderUtils::createTextureCube(
			resource_manager,
			driver,
			render::backend::Format::R32G32B32A32_SFLOAT,
			128,
			1,
			fullscreen_quad,
			getShader(config::Shaders::CubemapVertex),
			getShader(config::Shaders::DiffuseIrradianceCubemapFragment),
			environment_cubemaps[i]
		);
	}

	blue_noise = resource_manager->import<Texture>(config::blueNoise, driver);

	default_albedo = generateTexture(driver, 127, 127, 127);
	default_normal = generateTexture(driver, 127, 127, 255);
	default_roughness = generateTexture(driver, 255, 255, 255);
	default_metalness = generateTexture(driver, 0, 0, 0);
}

void ApplicationResources::shutdown()
{
	for (auto it : shaders)
		delete it.second;

	for (auto it : loaded_textures)
		resource_manager->destroy(it, driver);

	for (auto it : loaded_meshes)
		resource_manager->destroy(it, driver);

	shaders.clear();
	loaded_textures.clear();
	loaded_meshes.clear();

	resource_manager->destroy(baked_brdf, driver);

	for (int i = 0; i < environment_cubemaps.size(); ++i)
		resource_manager->destroy(environment_cubemaps[i], driver);
	
	for (int i = 0; i < irradiance_cubemaps.size(); ++i)
		resource_manager->destroy(irradiance_cubemaps[i], driver);

	hdr_cubemaps.clear();
	environment_cubemaps.clear();
	irradiance_cubemaps.clear();

	resource_manager->destroy(blue_noise, driver);

	resource_manager->destroy(fullscreen_quad, driver);
	resource_manager->destroy(skybox, driver);

	driver->destroyTexture(default_albedo);
	driver->destroyTexture(default_normal);
	driver->destroyTexture(default_roughness);
	driver->destroyTexture(default_metalness);

	default_albedo = nullptr;
	default_normal = nullptr;
	default_roughness = nullptr;
	default_metalness = nullptr;
}

/*
 */
void ApplicationResources::reloadShaders()
{
	for (int i = 0; i < config::shaders.size(); ++i)
		reloadShader(i);
}

Shader *ApplicationResources::getShader(int id) const
{
	auto it = shaders.find(id);
	if (it != shaders.end())
		return it->second;

	return nullptr;
}

Shader *ApplicationResources::loadShader(int id, render::backend::ShaderType type, const char *path)
{
	auto it = shaders.find(id);
	if (it != shaders.end())
	{
		std::cerr << "ApplicationResources::loadShader(): " << id << " is already taken by another shader" << std::endl;
		return nullptr;
	}

	Shader *shader = new Shader(driver, compiler);
	if (!shader->compileFromFile(type, path))
	{
		// TODO: log warning
	}

	shaders.insert(std::make_pair(id, shader));
	return shader;
}

bool ApplicationResources::reloadShader(int id)
{
	auto it = shaders.find(id);
	if (it == shaders.end())
		return false;

	return it->second->reload();
}

void ApplicationResources::unloadShader(int id)
{
	auto it = shaders.find(id);
	if (it == shaders.end())
		return;

	delete it->second;
	shaders.erase(it);
}

/*
 */
resources::ResourceHandle<Texture> ApplicationResources::loadTexture(const resources::URI &uri)
{
	resources::ResourceHandle<Texture> texture = resource_manager->import<Texture>(uri, driver);
	if (!texture.get())
		return resources::ResourceHandle<Texture>();

	loaded_textures.push_back(texture);
	return texture;
}

resources::ResourceHandle<Texture> ApplicationResources::loadTextureFromMemory(const uint8_t *data, size_t size)
{
	resources::ResourceHandle<Texture> texture = resource_manager->importFromMemory<Texture>(data, size, driver);
	if (!texture.get())
		return resources::ResourceHandle<Texture>();

	loaded_textures.push_back(texture);
	return texture;
}

/*
 */
resources::ResourceHandle<Mesh> ApplicationResources::loadMesh(const resources::URI &uri)
{
	resources::ResourceHandle<Mesh> result = resource_manager->import<Mesh>(uri, driver);
	if (!result.get())
		return resources::ResourceHandle<Mesh>();

	loaded_meshes.push_back(result);
	return result;
}

resources::ResourceHandle<Mesh> ApplicationResources::createMeshFromAssimp(const aiMesh *mesh)
{
	resources::ResourceHandle<Mesh> result = resource_manager->create<Mesh>();
	if (!result.get())
		return resources::ResourceHandle<Mesh>();

	resources::ResourcePipeline<Mesh>::createAssimp(result, driver, mesh);

	loaded_meshes.push_back(result);
	return result;
}

resources::ResourceHandle<Mesh> ApplicationResources::createMeshFromCGLTF(const cgltf_mesh *mesh)
{
	resources::ResourceHandle<Mesh> result = resource_manager->create<Mesh>();
	if (!result.get())
		return resources::ResourceHandle<Mesh>();

	resources::ResourcePipeline<Mesh>::createCGLTF(result, driver, mesh);

	loaded_meshes.push_back(result);
	return result;
}
