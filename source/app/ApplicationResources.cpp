#include "ApplicationResources.h"
#include "RenderUtils.h"

#include "IBLTexture.h"
#include "Mesh.h"
#include "RenderMaterial.h"
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

	static std::vector<render::backend::ShaderType> shader_types = {
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
	static std::vector<const char *> ibl_textures = {
		"assets/textures/environment/arctic.hdr",
		"assets/textures/environment/umbrellas.hdr",
		"assets/textures/environment/shanghai_bund_4k.hdr",
	};

	static const char *blue_noise = "assets/textures/blue_noise.png";
}

static resources::ResourceHandle<Texture> generateTexture(resources::ResourceManager *resource_manager, render::backend::Driver *driver, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t pixels[16] = {
		r, g, b, 255,
		r, g, b, 255,
		r, g, b, 255,
		r, g, b, 255,
	};

	resources::ResourceHandle<Texture> result = resource_manager->create<Texture>();
	resources::ResourcePipeline<Texture>::create2D(result, driver, render::backend::Format::R8G8B8A8_UNORM, 2, 2, 1, pixels);

	return result;
}

/*
 */
const char *ApplicationResources::getIBLTexturePath(int index) const
{
	assert(index >= 0 && index < config::ibl_textures.size());
	return config::ibl_textures[index];
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
		loadShader(config::shaders[i], config::shader_types[i]);

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

	blue_noise = resource_manager->import<Texture>(config::blue_noise, driver);

	default_albedo = generateTexture(resource_manager, driver, 127, 127, 127);
	default_normal = generateTexture(resource_manager, driver, 127, 127, 255);
	default_roughness = generateTexture(resource_manager, driver, 255, 255, 255);
	default_metalness = generateTexture(resource_manager, driver, 0, 0, 0);

	for (int i = 0; i < config::ibl_textures.size(); ++i)
		loadIBLTexture(config::ibl_textures[i]);
}

void ApplicationResources::shutdown()
{
	for (auto it : loaded_shaders)
		resource_manager->destroy(it, driver);

	for (auto it : loaded_textures)
		resource_manager->destroy(it, driver);

	for (auto it : loaded_ibl_textures)
		resource_manager->destroy(it, driver);

	for (auto it : loaded_meshes)
		resource_manager->destroy(it, driver);

	for (auto it : loaded_render_materials)
		resource_manager->destroy(it, driver);

	loaded_shaders.clear();
	loaded_textures.clear();
	loaded_ibl_textures.clear();
	loaded_meshes.clear();
	loaded_render_materials.clear();

	resource_manager->destroy(baked_brdf, driver);
	resource_manager->destroy(blue_noise, driver);

	resource_manager->destroy(fullscreen_quad, driver);
	resource_manager->destroy(skybox, driver);

	resource_manager->destroy(default_albedo, driver);
	resource_manager->destroy(default_normal, driver);
	resource_manager->destroy(default_roughness, driver);
	resource_manager->destroy(default_metalness, driver);
}

/*
 */
resources::ResourceHandle<Shader> ApplicationResources::loadShader(const resources::URI &uri, render::backend::ShaderType type)
{
	resources::ResourceHandle<Shader> shader = resource_manager->import<Shader>(uri, type, driver, compiler);
	if (!shader.get())
		return resources::ResourceHandle<Shader>();

	loaded_shaders.push_back(shader);
	return shader;
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
resources::ResourceHandle<IBLTexture> ApplicationResources::loadIBLTexture(const resources::URI &uri)
{
	resources::ResourcePipeline<IBLTexture>::ImportData import_data = {};
	import_data.format = render::backend::Format::R32G32B32A32_SFLOAT;
	import_data.cubemap_size = 128;

	import_data.fullscreen_quad = fullscreen_quad;
	import_data.baked_brdf = baked_brdf;
	import_data.cubemap_vertex = getShader(config::Shaders::CubemapVertex);
	import_data.equirectangular_projection_fragment = getShader(config::Shaders::EquirectangularProjectionFragment);
	import_data.prefiltered_specular_fragment = getShader(config::Shaders::PrefilteredSpecularCubemapFragment);
	import_data.diffuse_irradiance_fragment = getShader(config::Shaders::DiffuseIrradianceCubemapFragment);

	resources::ResourceHandle<IBLTexture> ibl_texture = resource_manager->import<IBLTexture>(uri, driver, import_data);
	if (!ibl_texture.get())
		return resources::ResourceHandle<IBLTexture>();

	loaded_ibl_textures.push_back(ibl_texture);
	return ibl_texture;
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

resources::ResourceHandle<RenderMaterial> ApplicationResources::createRenderMaterial(
	resources::ResourceHandle<Texture> albedo,
	resources::ResourceHandle<Texture> normal,
	resources::ResourceHandle<Texture> roughness,
	resources::ResourceHandle<Texture> metalness
)
{
	resources::ResourcePipeline<RenderMaterial>::ImportData import_data = {};

	import_data.albedo = albedo;
	import_data.normal = normal;
	import_data.roughness = roughness;
	import_data.metalness = metalness;

	resources::ResourceHandle<RenderMaterial> result = resource_manager->create<RenderMaterial>();
	if (!result.get())
		return resources::ResourceHandle<RenderMaterial>();

	resources::ResourcePipeline<RenderMaterial>::create(result, driver, import_data);

	loaded_render_materials.push_back(result);
	return result;
}
