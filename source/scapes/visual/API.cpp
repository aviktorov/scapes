#include "API.h"

#include <CubemapRenderer.h>
#include <Texture2DRenderer.h>

#include <scapes/foundation/game/World.h>
#include <scapes/foundation/game/Query.h>
#include <scapes/foundation/render/Device.h>
#include <scapes/foundation/resources/ResourceManager.h>

#include <algorithm>

namespace scapes::visual
{
	/*
	 */
	API *API::create(
		foundation::resources::ResourceManager *resource_manager,
		foundation::game::World *world,
		foundation::render::Device *device,
		foundation::shaders::Compiler *compiler
	)
	{
		return new APIImpl(resource_manager, world, device, compiler);
	}

	void API::destroy(API *api)
	{
		delete api;
	}

	/*
	 */
	APIImpl::APIImpl(
		foundation::resources::ResourceManager *resource_manager,
		foundation::game::World *world,
		foundation::render::Device *device,
		foundation::shaders::Compiler *compiler
	)
		: resource_manager(resource_manager), world(world), device(device), compiler(compiler)
	{
		unit_quad = generateMeshQuad(2.0f);
		unit_cube = generateMeshCube(2.0f);

		default_white = generateTexture(255, 255, 255);
		default_grey = generateTexture(127, 127, 127);
		default_black = generateTexture(0, 0, 0);
		default_normal = generateTexture(127, 127, 255);
	}

	APIImpl::~APIImpl()
	{
	}

	/*
	 */
	MeshHandle APIImpl::generateMeshQuad(float size)
	{
		constexpr uint32_t num_vertices = 4;
		constexpr uint32_t num_indices = 6;

		resources::Mesh::Vertex vertices[num_vertices];
		memset(vertices, 0, sizeof(resources::Mesh::Vertex) * num_vertices);

		float half_size = size * 0.5f;

		vertices[0].position = foundation::math::vec3(-half_size, -half_size, 0.0f);
		vertices[1].position = foundation::math::vec3( half_size, -half_size, 0.0f);
		vertices[2].position = foundation::math::vec3( half_size,  half_size, 0.0f);
		vertices[3].position = foundation::math::vec3(-half_size,  half_size, 0.0f);

		vertices[0].uv = foundation::math::vec2(0.0f, 0.0f);
		vertices[1].uv = foundation::math::vec2(1.0f, 0.0f);
		vertices[2].uv = foundation::math::vec2(1.0f, 1.0f);
		vertices[3].uv = foundation::math::vec2(0.0f, 1.0f);

		static uint32_t indices[num_indices] =
		{
			1, 0, 2, 3, 2, 0,
		};

		return resource_manager->create<resources::Mesh>(device, num_vertices, vertices, num_indices, indices);
	}

	MeshHandle APIImpl::generateMeshCube(float size)
	{
		constexpr uint32_t num_vertices = 8;
		constexpr uint32_t num_indices = 36;

		resources::Mesh::Vertex vertices[num_vertices];
		memset(vertices, 0, sizeof(resources::Mesh::Vertex) * num_vertices);

		float half_size = size * 0.5f;

		vertices[0].position = foundation::math::vec3(-half_size, -half_size, -half_size);
		vertices[1].position = foundation::math::vec3( half_size, -half_size, -half_size);
		vertices[2].position = foundation::math::vec3( half_size,  half_size, -half_size);
		vertices[3].position = foundation::math::vec3(-half_size,  half_size, -half_size);
		vertices[4].position = foundation::math::vec3(-half_size, -half_size,  half_size);
		vertices[5].position = foundation::math::vec3( half_size, -half_size,  half_size);
		vertices[6].position = foundation::math::vec3( half_size,  half_size,  half_size);
		vertices[7].position = foundation::math::vec3(-half_size,  half_size,  half_size);

		static uint32_t indices[num_indices] =
		{
			1, 5, 6, 6, 2, 1, // +x
			0, 1, 2, 2, 3, 0, // -x
			3, 2, 6, 6, 7, 3, // +y
			1, 0, 4, 4, 5, 1, // -y
			5, 4, 6, 4, 7, 6, // +z
			4, 0, 3, 3, 7, 4, // -z
		};

		return resource_manager->create<resources::Mesh>(device, num_vertices, vertices, num_indices, indices);
	}

	/*
	 */
	TextureHandle APIImpl::generateTexture(uint8_t r, uint8_t g, uint8_t b)
	{
		uint8_t pixels[16] = {
			r, g, b, 255,
			r, g, b, 255,
			r, g, b, 255,
			r, g, b, 255,
		};

		TextureHandle result = resource_manager->create<resources::Texture>(device);

		result->format = scapes::foundation::render::Format::R8G8B8A8_UNORM;
		result->width = 2;
		result->height = 2;
		result->depth = 1;
		result->mip_levels = 1;
		result->layers = 1;
		result->gpu_data = device->createTexture2D(result->width, result->height, result->mip_levels, result->format, pixels);

		return result;
	}
}
