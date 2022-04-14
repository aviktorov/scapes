#include "ApplicationResources.h"

#include <scapes/visual/Resources.h>
#include <scapes/visual/HdriImporter.h>
#include <scapes/visual/GlbImporter.h>

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
		BakeBRDFFragment,
	};

	// Shaders
	static std::vector<const char *> shaders =
	{
		"shaders/common/Default.vert",
		"shaders/common/DefaultCubemap.geom",
		"shaders/render_graph/utils/EquirectangularProjection.frag",
		"shaders/render_graph/utils/PrefilteredSpecularCubemap.frag",
		"shaders/render_graph/utils/DiffuseIrradianceCubemap.frag",
		"shaders/render_graph/utils/BakeBRDF.frag",
	};

	static std::vector<scapes::foundation::render::ShaderType> shader_types =
	{
		scapes::foundation::render::ShaderType::VERTEX,
		scapes::foundation::render::ShaderType::GEOMETRY,
		scapes::foundation::render::ShaderType::FRAGMENT,
		scapes::foundation::render::ShaderType::FRAGMENT,
		scapes::foundation::render::ShaderType::FRAGMENT,
		scapes::foundation::render::ShaderType::FRAGMENT,
	};

	// Textures
	static std::vector<const char *> ibl_textures =
	{
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
	scapes::foundation::resources::ResourceManager *resource_manager = visual_api->getResourceManager();
	assert(resource_manager);

	scapes::foundation::render::Device *device = visual_api->getDevice();
	assert(device);

	scapes::foundation::shaders::Compiler *compiler = visual_api->getCompiler();
	assert(compiler);

	scapes::foundation::game::World *world = visual_api->getWorld();
	assert(world);

	loaded_shaders.reserve(config::shaders.size());
	for (int i = 0; i < config::shaders.size(); ++i)
	{
		scapes::visual::ShaderHandle shader = resource_manager->fetch<scapes::visual::resources::Shader>(
			config::shaders[i],
			config::shader_types[i],
			device,
			compiler
		);

		loaded_shaders.push_back(shader);
	}

	scapes::visual::HdriImporter::CreateOptions options = {};
	options.resource_manager = resource_manager;
	options.world = world;
	options.device = device;
	options.compiler = compiler;

	options.unit_quad = visual_api->getUnitQuad();
	options.unit_cube = visual_api->getUnitCube();
	options.default_vertex = loaded_shaders[config::Shaders::DefaultVertex];
	options.default_cubemap_geometry = loaded_shaders[config::Shaders::DefaultCubemapGeometry];
	options.bake_brdf_fragment = loaded_shaders[config::Shaders::BakeBRDFFragment];
	options.equirectangular_projection_fragment = loaded_shaders[config::Shaders::EquirectangularProjectionFragment];
	options.diffuse_irradiance_fragment = loaded_shaders[config::Shaders::DiffuseIrradianceCubemapFragment];
	options.prefiltered_specular_fragment = loaded_shaders[config::Shaders::PrefilteredSpecularCubemapFragment];

	hdri_importer = scapes::visual::HdriImporter::create(options);
	baked_brdf = hdri_importer->bakeBRDF(scapes::foundation::render::Format::R16G16_SFLOAT, 512);

	for (int i = 0; i < config::ibl_textures.size(); ++i)
	{
		constexpr scapes::foundation::render::Format format = scapes::foundation::render::Format::R32G32B32A32_SFLOAT;
		constexpr uint32_t size = 128;

		scapes::visual::IBLTextureHandle ibl_texture = hdri_importer->import(config::ibl_textures[i], format, size, baked_brdf);
		loaded_ibl_textures.push_back(ibl_texture);
	}

	default_material = resource_manager->create<scapes::visual::resources::RenderMaterial>(
		visual_api->getGreyTexture(),
		visual_api->getNormalTexture(),
		visual_api->getWhiteTexture(),
		visual_api->getBlackTexture(),
		device
	);

	glb_importer = scapes::visual::GlbImporter::create(resource_manager, world, device);
	glb_importer->import("scenes/sphere.glb", default_material);
}

void ApplicationResources::shutdown()
{
	scapes::visual::GlbImporter::destroy(glb_importer);
	glb_importer = nullptr;

	scapes::visual::HdriImporter::destroy(hdri_importer);
	hdri_importer = nullptr;

	loaded_shaders.clear();
	loaded_ibl_textures.clear();
}
