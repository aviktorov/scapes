#include "ApplicationResources.h"

#include <scapes/visual/Resources.h>

#include <cassert>
#include <iostream>

namespace config
{
	enum Shaders
	{
		DefaultVertex = 0,
		DefaultCubemapGeometry,
		EquirectangularProjectionFragment,
		PrefilteredSpecularCubemapFragment,
		DiffuseIrradianceCubemapFragment,
		BakedBRDFFragment,
	};

	// Shaders
	static std::vector<const char *> shaders = {
		"shaders/common/Default.vert",
		"shaders/common/DefaultCubemap.geom",
		"shaders/render_graph/utils/EquirectangularProjection.frag",
		"shaders/render_graph/utils/PrefilteredSpecularCubemap.frag",
		"shaders/render_graph/utils/DiffuseIrradianceCubemap.frag",
		"shaders/render_graph/utils/BakedBRDF.frag",
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
		"textures/environment/debug.jpg",
		"textures/environment/arctic.hdr",
		"textures/environment/umbrellas.hdr",
		"textures/environment/shanghai_bund_4k.hdr",
	};
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
		loaded_shaders[config::Shaders::DefaultVertex],
		loaded_shaders[config::Shaders::BakedBRDFFragment]
	);

	scapes::foundation::render::Device *device = visual_api->getDevice();
	device->setTextureSamplerWrapMode(baked_brdf->gpu_data, scapes::foundation::render::SamplerWrapMode::CLAMP_TO_EDGE);

	scapes::visual::IBLTextureCreateData create_data = {};
	create_data.format = scapes::foundation::render::Format::R32G32B32A32_SFLOAT;
	create_data.cubemap_size = 128;

	create_data.baked_brdf = baked_brdf;
	create_data.cubemap_vertex = loaded_shaders[config::Shaders::DefaultVertex];
	create_data.cubemap_geometry = loaded_shaders[config::Shaders::DefaultCubemapGeometry];
	create_data.equirectangular_projection_fragment = loaded_shaders[config::Shaders::EquirectangularProjectionFragment];
	create_data.prefiltered_specular_fragment = loaded_shaders[config::Shaders::PrefilteredSpecularCubemapFragment];
	create_data.diffuse_irradiance_fragment = loaded_shaders[config::Shaders::DiffuseIrradianceCubemapFragment];

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
