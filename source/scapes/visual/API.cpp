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
		const foundation::io::URI &default_vertex_shader_uri,
		const foundation::io::URI &default_cubemap_geometry_shader_uri,
		foundation::resources::ResourceManager *resource_manager,
		foundation::game::World *world,
		foundation::render::Device *device,
		foundation::shaders::Compiler *compiler
	)
	{
		return new APIImpl(default_vertex_shader_uri, default_cubemap_geometry_shader_uri, resource_manager, world, device, compiler);
	}

	void API::destroy(API *api)
	{
		delete api;
	}

	/*
	 */
	APIImpl::APIImpl(
		const foundation::io::URI &default_vertex_shader_uri,
		const foundation::io::URI &default_cubemap_geometry_shader_uri,
		foundation::resources::ResourceManager *resource_manager,
		foundation::game::World *world,
		foundation::render::Device *device,
		foundation::shaders::Compiler *compiler
	)
		: resource_manager(resource_manager), world(world), device(device), compiler(compiler)
	{
		default_vertex = resource_manager->fetch<resources::Shader>(
			default_vertex_shader_uri,
			foundation::render::ShaderType::VERTEX,
			device,
			compiler
		);

		default_cubemap_geometry = resource_manager->fetch<resources::Shader>(
			default_cubemap_geometry_shader_uri,
			foundation::render::ShaderType::GEOMETRY,
			device,
			compiler
		);

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

	TextureHandle APIImpl::createTexture2D(
		foundation::render::Format format,
		uint32_t width,
		uint32_t height,
		uint32_t num_mips,
		const void *data,
		uint32_t num_data_mipmaps
	)
	{
		TextureHandle result = resource_manager->create<resources::Texture>(device);

		result->format = format;
		result->width = width;
		result->height = height;
		result->mip_levels = num_mips;
		result->layers = 1;
		result->gpu_data = device->createTexture2D(width, height, num_mips, format, data, num_data_mipmaps);

		return result;
	}

	void APIImpl::renderTexture2D(
		TextureHandle target,
		ShaderHandle fragment_shader
	)
	{
		Texture2DRenderer renderer(device);
		renderer.init(target.get());
		renderer.render(unit_quad.get(), default_vertex.get(), fragment_shader.get());
	}

	TextureHandle APIImpl::createTextureCube(
		foundation::render::Format format,
		uint32_t size,
		uint32_t num_mips,
		const void *data,
		uint32_t num_data_mipmaps
	)
	{
		TextureHandle result = resource_manager->create<resources::Texture>(device);

		result->format = format;
		result->width = size;
		result->height = size;
		result->mip_levels = num_mips;
		result->layers = 6;
		result->gpu_data = device->createTextureCube(size, num_mips, format, data, num_data_mipmaps);

		return result;
	}

	void APIImpl::renderTextureCube(
		TextureHandle target,
		uint32_t target_mip,
		ShaderHandle fragment_shader,
		TextureHandle input_texture,
		size_t size,
		const void *data
	)
	{
		CubemapRenderer renderer(device);
		renderer.init(target.get(), target_mip);
		renderer.render(
			unit_cube.get(),
			default_vertex.get(),
			default_cubemap_geometry.get(),
			fragment_shader.get(),
			input_texture.get(),
			static_cast<uint8_t>(size),
			reinterpret_cast<const uint8_t *>(data)
		);
	}

	IBLTextureHandle APIImpl::loadIBLTexture(
		const foundation::io::URI &uri,
		const IBLTextureCreateData &create_data
	)
	{
		TextureHandle hdri_texture = resource_manager->load<resources::Texture>(uri, device);
		if (hdri_texture.get() == nullptr)
			return IBLTextureHandle();

		uint32_t mips = static_cast<int>(std::floor(std::log2(create_data.cubemap_size)) + 1);

		resources::Texture diffuse_irradiance = {};
		diffuse_irradiance.format = create_data.format;
		diffuse_irradiance.width = create_data.cubemap_size;
		diffuse_irradiance.height = create_data.cubemap_size;
		diffuse_irradiance.mip_levels = 1;
		diffuse_irradiance.layers = 6;
		diffuse_irradiance.gpu_data = device->createTextureCube(create_data.cubemap_size, 1, create_data.format);

		resources::Texture temp_cubemap = {};
		temp_cubemap.format = create_data.format;
		temp_cubemap.width = create_data.cubemap_size;
		temp_cubemap.height = create_data.cubemap_size;
		temp_cubemap.mip_levels = 1;
		temp_cubemap.layers = 6;
		temp_cubemap.gpu_data = device->createTextureCube(create_data.cubemap_size, 1, create_data.format);

		resources::Texture prefiltered_specular = {};
		prefiltered_specular.format = create_data.format;
		prefiltered_specular.width = create_data.cubemap_size;
		prefiltered_specular.height = create_data.cubemap_size;
		prefiltered_specular.mip_levels = mips;
		prefiltered_specular.layers = 6;
		prefiltered_specular.gpu_data = device->createTextureCube(create_data.cubemap_size, mips, create_data.format);

		CubemapRenderer renderer(device);
		renderer.init(&temp_cubemap, 0);
		renderer.render(
			unit_cube.get(),
			default_vertex.get(),
			default_cubemap_geometry.get(),
			create_data.equirectangular_projection_fragment.get(),
			hdri_texture.get()
		);
		renderer.shutdown();

		resource_manager->destroy(hdri_texture);

		for (uint32_t mip = 0; mip < prefiltered_specular.mip_levels; ++mip)
		{
			float roughness = static_cast<float>(mip) / prefiltered_specular.mip_levels;

			uint8_t size = static_cast<uint8_t>(sizeof(float));
			const uint8_t *data = reinterpret_cast<const uint8_t *>(&roughness);

			renderer.init(&prefiltered_specular, mip);
			renderer.render(
				unit_cube.get(),
				default_vertex.get(),
				default_cubemap_geometry.get(),
				create_data.prefiltered_specular_fragment.get(),
				&temp_cubemap,
				size,
				data
			);
			renderer.shutdown();
		}

		renderer.init(&diffuse_irradiance, 0);
		renderer.render(
			unit_cube.get(),
			default_vertex.get(),
			default_cubemap_geometry.get(),
			create_data.diffuse_irradiance_fragment.get(),
			&temp_cubemap
		);
		renderer.shutdown();

		device->destroyTexture(temp_cubemap.gpu_data);

		IBLTextureHandle result = resource_manager->create<resources::IBLTexture>(device);

		result->diffuse_irradiance_cubemap = diffuse_irradiance.gpu_data;
		result->prefiltered_specular_cubemap = prefiltered_specular.gpu_data;

		result->baked_brdf = create_data.baked_brdf;

		result->bindings = device->createBindSet();
		device->bindTexture(result->bindings, 0, result->baked_brdf->gpu_data);
		device->bindTexture(result->bindings, 1, result->prefiltered_specular_cubemap);
		device->bindTexture(result->bindings, 2, result->diffuse_irradiance_cubemap);

		return result;
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

		return createTexture2D(scapes::foundation::render::Format::R8G8B8A8_UNORM, 2, 2, 1, pixels);
	}
}
