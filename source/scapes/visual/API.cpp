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
	static void uploadToGPU(
		foundation::render::Device *device,
		MeshHandle mesh
	)
	{
		assert(device);
		assert(mesh.get());
		assert(mesh->num_vertices);
		assert(mesh->num_indices);
		assert(mesh->vertices);
		assert(mesh->indices);

		static foundation::render::VertexAttribute mesh_attributes[6] =
		{
			{ foundation::render::Format::R32G32B32_SFLOAT, offsetof(resources::Mesh::Vertex, position) },
			{ foundation::render::Format::R32G32_SFLOAT, offsetof(resources::Mesh::Vertex, uv) },
			{ foundation::render::Format::R32G32B32A32_SFLOAT, offsetof(resources::Mesh::Vertex, tangent) },
			{ foundation::render::Format::R32G32B32_SFLOAT, offsetof(resources::Mesh::Vertex, binormal) },
			{ foundation::render::Format::R32G32B32_SFLOAT, offsetof(resources::Mesh::Vertex, normal) },
			{ foundation::render::Format::R32G32B32A32_SFLOAT, offsetof(resources::Mesh::Vertex, color) },
		};

		mesh->vertex_buffer = device->createVertexBuffer(
			foundation::render::BufferType::STATIC,
			sizeof(resources::Mesh::Vertex), mesh->num_vertices,
			6, mesh_attributes,
			mesh->vertices
		);

		mesh->index_buffer = device->createIndexBuffer(
			foundation::render::BufferType::STATIC,
			foundation::render::IndexFormat::UINT32,
			mesh->num_indices,
			mesh->indices
		);
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
		unit_quad = createMeshQuad(2.0f);
		unit_cube = createMeshCube(2.0f);
	}

	APIImpl::~APIImpl()
	{
		uri_shader_lookup.clear();
		shader_uri_lookup.clear();

		uri_texture_lookup.clear();
		texture_uri_lookup.clear();
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

	TextureHandle APIImpl::createTexture2D(
		foundation::render::Format format,
		uint32_t width,
		uint32_t height,
		ShaderHandle vertex_shader,
		ShaderHandle fragment_shader
	)
	{
		TextureHandle result = createTexture2D(format, width, height, 1);

		Texture2DRenderer renderer(device);
		renderer.init(result.get());
		renderer.render(unit_quad.get(), vertex_shader.get(), fragment_shader.get());

		return result;
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

	TextureHandle APIImpl::loadTextureFromMemory(
		const uint8_t *data,
		size_t size
	)
	{
		TextureHandle result = resource_manager->importFromMemory<resources::Texture>(data, size, device);

		return result;
	}

	TextureHandle APIImpl::loadTexture(
		const foundation::io::URI &uri
	)
	{
		std::string uri_string = std::string(uri);
		auto it = uri_texture_lookup.find(uri_string);
		if (it != uri_texture_lookup.end())
			return it->second;

		TextureHandle result = resource_manager->import<resources::Texture>(uri, device);

		if (result.get())
		{
			uri_texture_lookup.insert({uri_string, result});
			texture_uri_lookup.insert({result, uri_string});
		}

		return result;
	}

	foundation::io::URI APIImpl::getTextureUri(
		TextureHandle handle
	) const
	{
		auto it = texture_uri_lookup.find(handle);
		if (it == texture_uri_lookup.end())
			return nullptr;

		return it->second.c_str();
	}

	ShaderHandle APIImpl::loadShaderFromMemory(
		const uint8_t *data,
		size_t size,
		foundation::render::ShaderType shader_type
	)
	{
		ShaderHandle result = resource_manager->importFromMemory<resources::Shader>(data, size, shader_type, device, compiler);

		return result;
	}

	ShaderHandle APIImpl::loadShader(
		const foundation::io::URI &uri,
		foundation::render::ShaderType shader_type
	)
	{
		std::string uri_string = std::string(uri);
		auto it = uri_shader_lookup.find(uri_string);
		if (it != uri_shader_lookup.end())
			return it->second;

		ShaderHandle result = resource_manager->import<resources::Shader>(uri, shader_type, device, compiler);

		if (result.get())
		{
			uri_shader_lookup.insert({uri_string, result});
			shader_uri_lookup.insert({result, uri_string});
		}

		return result;
	}

	foundation::io::URI APIImpl::getShaderUri(
		ShaderHandle handle
	) const
	{
		auto it = shader_uri_lookup.find(handle);
		if (it == shader_uri_lookup.end())
			return nullptr;

		return it->second.c_str();
	}

	MeshHandle APIImpl::createMesh(
		uint32_t num_vertices,
		resources::Mesh::Vertex *vertices,
		uint32_t num_indices,
		uint32_t *indices
	)
	{
		MeshHandle result = resource_manager->create<resources::Mesh>(device);

		result->num_vertices = num_vertices;
		result->num_indices = num_indices;

		// TODO: use subresource pools
		result->vertices = new resources::Mesh::Vertex[result->num_vertices];
		result->indices = new uint32_t[result->num_indices];

		memcpy(result->vertices, vertices, sizeof(resources::Mesh::Vertex) * result->num_vertices);
		memcpy(result->indices, indices, sizeof(uint32_t) * result->num_indices);
		uploadToGPU(device, result);

		return result;
	}

	MeshHandle APIImpl::createMeshQuad(
		float size
	)
	{
		MeshHandle result = resource_manager->create<resources::Mesh>(device);

		result->num_vertices = 4;
		result->num_indices = 6;

		// TODO: use subresource pools
		result->vertices = new resources::Mesh::Vertex[result->num_vertices];
		result->indices = new uint32_t[result->num_indices];

		memset(result->vertices, 0, sizeof(resources::Mesh::Vertex) * result->num_vertices);

		float half_size = size * 0.5f;

		result->vertices[0].position = foundation::math::vec3(-half_size, -half_size, 0.0f);
		result->vertices[1].position = foundation::math::vec3( half_size, -half_size, 0.0f);
		result->vertices[2].position = foundation::math::vec3( half_size,  half_size, 0.0f);
		result->vertices[3].position = foundation::math::vec3(-half_size,  half_size, 0.0f);

		result->vertices[0].uv = foundation::math::vec2(0.0f, 0.0f);
		result->vertices[1].uv = foundation::math::vec2(1.0f, 0.0f);
		result->vertices[2].uv = foundation::math::vec2(1.0f, 1.0f);
		result->vertices[3].uv = foundation::math::vec2(0.0f, 1.0f);

		static uint32_t quad_indices[] =
		{
			1, 0, 2, 3, 2, 0,
		};

		memcpy(result->indices, quad_indices, sizeof(uint32_t) * result->num_indices);
		uploadToGPU(device, result);

		return result;
	}

	MeshHandle APIImpl::createMeshCube(
		float size
	)
	{
		MeshHandle result = resource_manager->create<resources::Mesh>(device);

		result->num_vertices = 8;
		result->num_indices = 36;

		// TODO: use subresource pools
		result->vertices = new resources::Mesh::Vertex[result->num_vertices];
		result->indices = new uint32_t[result->num_indices];

		memset(result->vertices, 0, sizeof(resources::Mesh::Vertex) * result->num_vertices);

		float half_size = size * 0.5f;

		result->vertices[0].position = foundation::math::vec3(-half_size, -half_size, -half_size);
		result->vertices[1].position = foundation::math::vec3( half_size, -half_size, -half_size);
		result->vertices[2].position = foundation::math::vec3( half_size,  half_size, -half_size);
		result->vertices[3].position = foundation::math::vec3(-half_size,  half_size, -half_size);
		result->vertices[4].position = foundation::math::vec3(-half_size, -half_size,  half_size);
		result->vertices[5].position = foundation::math::vec3( half_size, -half_size,  half_size);
		result->vertices[6].position = foundation::math::vec3( half_size,  half_size,  half_size);
		result->vertices[7].position = foundation::math::vec3(-half_size,  half_size,  half_size);

		static uint32_t skybox_indices[] =
		{
			1, 5, 6, 6, 2, 1, // +x
			0, 1, 2, 2, 3, 0, // -x
			3, 2, 6, 6, 7, 3, // +y
			1, 0, 4, 4, 5, 1, // -y
			5, 4, 6, 4, 7, 6, // +z
			4, 0, 3, 3, 7, 4, // -z
		};

		memcpy(result->indices, skybox_indices, sizeof(uint32_t) * result->num_indices);
		uploadToGPU(device, result);

		return result;
	}

	IBLTextureHandle APIImpl::importIBLTexture(
		const foundation::io::URI &uri,
		const IBLTextureCreateData &create_data
	)
	{
		TextureHandle hdri_texture = resource_manager->import<resources::Texture>(uri, device);
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
			create_data.cubemap_vertex.get(),
			create_data.cubemap_geometry.get(),
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
				create_data.cubemap_vertex.get(),
				create_data.cubemap_geometry.get(),
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
			create_data.cubemap_vertex.get(),
			create_data.cubemap_geometry.get(),
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

	RenderMaterialHandle APIImpl::createRenderMaterial(
		TextureHandle albedo,
		TextureHandle normal,
		TextureHandle roughness,
		TextureHandle metalness
	)
	{
		RenderMaterialHandle result = resource_manager->create<resources::RenderMaterial>(device);

		result->albedo = albedo;
		result->normal = normal;
		result->roughness = roughness;
		result->metalness = metalness;

		result->bindings = device->createBindSet();
		device->bindTexture(result->bindings, 0, result->albedo->gpu_data);
		device->bindTexture(result->bindings, 1, result->normal->gpu_data);
		device->bindTexture(result->bindings, 2, result->roughness->gpu_data);
		device->bindTexture(result->bindings, 3, result->metalness->gpu_data);

		return result;
	}
}
