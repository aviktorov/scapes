#include "ApplicationResources.h"
#include "RenderUtils.h"

#include <scapes/visual/Resources.h>

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

static scapes::visual::TextureHandle generateTexture(scapes::visual::API *visual_api, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t pixels[16] = {
		r, g, b, 255,
		r, g, b, 255,
		r, g, b, 255,
		r, g, b, 255,
	};

	return visual_api->createTexture2D(render::backend::Format::R8G8B8A8_UNORM, 2, 2, 1, pixels);
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
	loaded_shaders.reserve(config::shaders.size());
	for (int i = 0; i < config::shaders.size(); ++i)
	{
		scapes::visual::ShaderHandle shader = visual_api->loadShader(config::shaders[i], config::shader_types[i]);
		loaded_shaders.push_back(shader);
	}

	fullscreen_quad = visual_api->createMeshQuad(2.0f);
	skybox = visual_api->createMeshSkybox(10000.0f);

	baked_brdf = visual_api->createTexture2D(
		render::backend::Format::R16G16_SFLOAT,
		512,
		512,
		fullscreen_quad,
		getShader(config::Shaders::FullscreenQuadVertex),
		getShader(config::Shaders::BakedBRDFFragment)
	);

	driver->setTextureSamplerWrapMode(baked_brdf->gpu_data, render::backend::SamplerWrapMode::CLAMP_TO_EDGE);

	blue_noise = visual_api->loadTexture(config::blue_noise);

	default_albedo = generateTexture(visual_api, 127, 127, 127);
	default_normal = generateTexture(visual_api, 127, 127, 255);
	default_roughness = generateTexture(visual_api, 255, 255, 255);
	default_metalness = generateTexture(visual_api, 0, 0, 0);

	scapes::visual::IBLTextureCreateData create_data = {};
	create_data.format = render::backend::Format::R32G32B32A32_SFLOAT;
	create_data.cubemap_size = 128;

	create_data.fullscreen_quad = fullscreen_quad;
	create_data.baked_brdf = baked_brdf;
	create_data.cubemap_vertex = getShader(config::Shaders::CubemapVertex);
	create_data.equirectangular_projection_fragment = getShader(config::Shaders::EquirectangularProjectionFragment);
	create_data.prefiltered_specular_fragment = getShader(config::Shaders::PrefilteredSpecularCubemapFragment);
	create_data.diffuse_irradiance_fragment = getShader(config::Shaders::DiffuseIrradianceCubemapFragment);

	for (int i = 0; i < config::ibl_textures.size(); ++i)
	{
		scapes::visual::IBLTextureHandle ibl_texture = visual_api->importIBLTexture(config::ibl_textures[i], create_data);
		loaded_ibl_textures.push_back(ibl_texture);
	}
}

void ApplicationResources::shutdown()
{
	loaded_shaders.clear();
	loaded_ibl_textures.clear();
}
