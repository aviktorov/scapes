#include "ApplicationResources.h"

#include <scapes/visual/Resources.h>

#include <cassert>
#include <iostream>

namespace config
{
	// Shaders
	static std::vector<const char *> shaders = {
		"assets/shaders/common/Default.vert",
		"assets/shaders/common/DefaultCubemap.geom",
		"assets/shaders/render_graph/utils/EquirectangularProjection.frag",
		"assets/shaders/render_graph/utils/PrefilteredSpecularCubemap.frag",
		"assets/shaders/render_graph/utils/DiffuseIrradianceCubemap.frag",
		"assets/shaders/render_graph/utils/BakedBRDF.frag",
	};

	static std::vector<scapes::foundation::render::ShaderType> shader_types = {
		scapes::foundation::render::ShaderType::VERTEX,
		scapes::foundation::render::ShaderType::GEOMETRY,
		scapes::foundation::render::ShaderType::FRAGMENT,
		scapes::foundation::render::ShaderType::FRAGMENT,
		scapes::foundation::render::ShaderType::FRAGMENT,
		scapes::foundation::render::ShaderType::FRAGMENT,
	};

	// Textures
	static std::vector<const char *> ibl_textures = {
		"assets/textures/environment/debug.jpg",
		"assets/textures/environment/arctic.hdr",
		"assets/textures/environment/umbrellas.hdr",
		"assets/textures/environment/shanghai_bund_4k.hdr",
	};
}

static scapes::visual::TextureHandle generateTexture(scapes::visual::API *visual_api, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t pixels[16] = {
		r, g, b, 255,
		r, g, b, 255,
		r, g, b, 255,
		r, g, b, 255,
	};

	return visual_api->createTexture2D(scapes::foundation::render::Format::R8G8B8A8_UNORM, 2, 2, 1, pixels);
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

	baked_brdf = visual_api->createTexture2D(
		scapes::foundation::render::Format::R16G16_SFLOAT,
		512,
		512,
		getShader(config::Shaders::DefaultVertex),
		getShader(config::Shaders::BakedBRDFFragment)
	);

	scapes::foundation::render::Device *device = visual_api->getDevice();
	device->setTextureSamplerWrapMode(baked_brdf->gpu_data, scapes::foundation::render::SamplerWrapMode::CLAMP_TO_EDGE);

	default_albedo = generateTexture(visual_api, 127, 127, 127);
	default_normal = generateTexture(visual_api, 127, 127, 255);
	default_roughness = generateTexture(visual_api, 255, 255, 255);
	default_metalness = generateTexture(visual_api, 0, 0, 0);

	scapes::visual::IBLTextureCreateData create_data = {};
	create_data.format = scapes::foundation::render::Format::R32G32B32A32_SFLOAT;
	create_data.cubemap_size = 128;

	create_data.baked_brdf = baked_brdf;
	create_data.cubemap_vertex = getShader(config::Shaders::DefaultVertex);
	create_data.cubemap_geometry = getShader(config::Shaders::DefaultCubemapGeometry);
	create_data.equirectangular_projection_fragment = getShader(config::Shaders::EquirectangularProjectionFragment);
	create_data.prefiltered_specular_fragment = getShader(config::Shaders::PrefilteredSpecularCubemapFragment);
	create_data.diffuse_irradiance_fragment = getShader(config::Shaders::DiffuseIrradianceCubemapFragment);

	for (int i = 0; i < config::ibl_textures.size(); ++i)
	{
		scapes::visual::IBLTextureHandle ibl_texture = visual_api->loadIBLTexture(config::ibl_textures[i], create_data);
		loaded_ibl_textures.push_back(ibl_texture);
	}
}

void ApplicationResources::shutdown()
{
	loaded_shaders.clear();
	loaded_ibl_textures.clear();
}
