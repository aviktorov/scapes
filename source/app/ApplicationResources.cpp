#include "ApplicationResources.h"

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

	static std::vector<scapes::visual::hardware::ShaderType> shader_types =
	{
		scapes::visual::hardware::ShaderType::VERTEX,
		scapes::visual::hardware::ShaderType::GEOMETRY,
		scapes::visual::hardware::ShaderType::FRAGMENT,
		scapes::visual::hardware::ShaderType::FRAGMENT,
		scapes::visual::hardware::ShaderType::FRAGMENT,
		scapes::visual::hardware::ShaderType::FRAGMENT,
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
	unit_quad = generateMeshQuad(2.0f);
	unit_cube = generateMeshCube(2.0f);

	default_white = generateTexture(255, 255, 255);
	default_grey = generateTexture(127, 127, 127);
	default_black = generateTexture(0, 0, 0);
	default_normal = generateTexture(127, 127, 255);

	default_material = resource_manager->create<scapes::visual::RenderMaterial>(
		default_grey,
		default_normal,
		default_white,
		default_black,
		device
	);

	loaded_shaders.reserve(config::shaders.size());
	for (int i = 0; i < config::shaders.size(); ++i)
	{
		scapes::visual::ShaderHandle shader = resource_manager->fetch<scapes::visual::Shader>(
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

	options.unit_quad = unit_quad;
	options.unit_cube = unit_cube;
	options.default_vertex = loaded_shaders[config::Shaders::DefaultVertex];
	options.default_cubemap_geometry = loaded_shaders[config::Shaders::DefaultCubemapGeometry];
	options.bake_brdf_fragment = loaded_shaders[config::Shaders::BakeBRDFFragment];
	options.equirectangular_projection_fragment = loaded_shaders[config::Shaders::EquirectangularProjectionFragment];
	options.diffuse_irradiance_fragment = loaded_shaders[config::Shaders::DiffuseIrradianceCubemapFragment];
	options.prefiltered_specular_fragment = loaded_shaders[config::Shaders::PrefilteredSpecularCubemapFragment];

	hdri_importer = scapes::visual::HdriImporter::create(options);
	baked_brdf = hdri_importer->bakeBRDF(scapes::visual::hardware::Format::R16G16_SFLOAT, 512);

	for (int i = 0; i < config::ibl_textures.size(); ++i)
	{
		constexpr scapes::visual::hardware::Format format = scapes::visual::hardware::Format::R32G32B32A32_SFLOAT;
		constexpr uint32_t size = 128;

		scapes::visual::IBLTextureHandle ibl_texture = hdri_importer->import(config::ibl_textures[i], format, size, baked_brdf);
		loaded_ibl_textures.push_back(ibl_texture);
	}

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

/*
 */
scapes::visual::MeshHandle ApplicationResources::generateMeshQuad(float size)
{
	constexpr uint32_t num_vertices = 4;
	constexpr uint32_t num_indices = 6;

	scapes::visual::Mesh::Vertex vertices[num_vertices];
	memset(vertices, 0, sizeof(scapes::visual::Mesh::Vertex) * num_vertices);

	float half_size = size * 0.5f;

	vertices[0].position = scapes::foundation::math::vec3(-half_size, -half_size, 0.0f);
	vertices[1].position = scapes::foundation::math::vec3( half_size, -half_size, 0.0f);
	vertices[2].position = scapes::foundation::math::vec3( half_size,  half_size, 0.0f);
	vertices[3].position = scapes::foundation::math::vec3(-half_size,  half_size, 0.0f);

	vertices[0].uv = scapes::foundation::math::vec2(0.0f, 0.0f);
	vertices[1].uv = scapes::foundation::math::vec2(1.0f, 0.0f);
	vertices[2].uv = scapes::foundation::math::vec2(1.0f, 1.0f);
	vertices[3].uv = scapes::foundation::math::vec2(0.0f, 1.0f);

	static uint32_t indices[num_indices] =
	{
		1, 0, 2, 3, 2, 0,
	};

	return resource_manager->create<scapes::visual::Mesh>(device, num_vertices, vertices, num_indices, indices);
}

scapes::visual::MeshHandle ApplicationResources::generateMeshCube(float size)
{
	constexpr uint32_t num_vertices = 8;
	constexpr uint32_t num_indices = 36;

	scapes::visual::Mesh::Vertex vertices[num_vertices];
	memset(vertices, 0, sizeof(scapes::visual::Mesh::Vertex) * num_vertices);

	float half_size = size * 0.5f;

	vertices[0].position = scapes::foundation::math::vec3(-half_size, -half_size, -half_size);
	vertices[1].position = scapes::foundation::math::vec3( half_size, -half_size, -half_size);
	vertices[2].position = scapes::foundation::math::vec3( half_size,  half_size, -half_size);
	vertices[3].position = scapes::foundation::math::vec3(-half_size,  half_size, -half_size);
	vertices[4].position = scapes::foundation::math::vec3(-half_size, -half_size,  half_size);
	vertices[5].position = scapes::foundation::math::vec3( half_size, -half_size,  half_size);
	vertices[6].position = scapes::foundation::math::vec3( half_size,  half_size,  half_size);
	vertices[7].position = scapes::foundation::math::vec3(-half_size,  half_size,  half_size);

	static uint32_t indices[num_indices] =
	{
		1, 5, 6, 6, 2, 1, // +x
		0, 1, 2, 2, 3, 0, // -x
		3, 2, 6, 6, 7, 3, // +y
		1, 0, 4, 4, 5, 1, // -y
		5, 4, 6, 4, 7, 6, // +z
		4, 0, 3, 3, 7, 4, // -z
	};

	return resource_manager->create<scapes::visual::Mesh>(device, num_vertices, vertices, num_indices, indices);
}

/*
 */
scapes::visual::TextureHandle ApplicationResources::generateTexture(uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t pixels[16] =
	{
		r, g, b, 255,
		r, g, b, 255,
		r, g, b, 255,
		r, g, b, 255,
	};

	scapes::visual::TextureHandle result = resource_manager->create<scapes::visual::Texture>(device);

	result->format = scapes::visual::hardware::Format::R8G8B8A8_UNORM;
	result->width = 2;
	result->height = 2;
	result->depth = 1;
	result->mip_levels = 1;
	result->layers = 1;
	result->gpu_data = device->createTexture2D(result->width, result->height, result->mip_levels, result->format, pixels);

	return result;
}
