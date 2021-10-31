#include "API.h"
#include <RenderUtils.h>

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
		renderable_query = new foundation::game::Query<components::Transform, components::Renderable>(world);
		skylight_query = new foundation::game::Query<components::SkyLight>(world);
	}

	APIImpl::~APIImpl()
	{
		delete renderable_query;
		delete skylight_query;

		for (TextureHandle handle : managed_textures)
			resource_manager->destroy(handle, device);

		for (ShaderHandle handle : managed_shaders)
			resource_manager->destroy(handle, device);

		for (MeshHandle handle : managed_meshes)
			resource_manager->destroy(handle, device);

		for (RenderMaterialHandle handle : managed_render_materials)
			resource_manager->destroy(handle, device);

		for (IBLTextureHandle handle : managed_ibl_textures)
			resource_manager->destroy(handle, device);

		managed_textures.clear();
		managed_shaders.clear();
		managed_meshes.clear();
		managed_render_materials.clear();
		managed_ibl_textures.clear();
	}

	void APIImpl::drawRenderables(
		uint8_t material_binding,
		foundation::render::PipelineState *pipeline_state,
		foundation::render::CommandBuffer *command_buffer
	)
	{
		renderable_query->begin();

		while (renderable_query->next())
		{
			uint32_t num_items = renderable_query->getNumComponents();
			components::Transform *transforms = renderable_query->getComponents<components::Transform>(0);
			components::Renderable *renderables = renderable_query->getComponents<components::Renderable>(1);

			for (uint32_t i = 0; i < num_items; ++i)
			{
				const components::Transform &transform = transforms[i];
				const components::Renderable &renderable = renderables[i];
				const foundation::math::mat4 &node_transform = transform.transform;

				foundation::render::BindSet *material_bindings = renderable.material->bindings;

				device->clearVertexStreams(pipeline_state);
				device->setVertexStream(pipeline_state, 0, renderable.mesh->vertex_buffer);

				device->setBindSet(pipeline_state, material_binding, material_bindings);
				device->setPushConstants(pipeline_state, static_cast<uint8_t>(sizeof(foundation::math::mat4)), &node_transform);

				device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, renderable.mesh->index_buffer, renderable.mesh->num_indices);
			}
		}
	}

	void APIImpl::drawSkyLights(
		uint8_t light_binding,
		foundation::render::PipelineState *pipeline_state,
		foundation::render::CommandBuffer *command_buffer
	)
	{
		skylight_query->begin();

		while (skylight_query->next())
		{
			uint32_t num_items = skylight_query->getNumComponents();
			components::SkyLight *skylights = skylight_query->getComponents<components::SkyLight>(0);

			for (uint32_t i = 0; i < num_items; ++i)
			{
				const components::SkyLight &light = skylights[i];

				device->clearShaders(pipeline_state);
				device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, light.vertex_shader->shader);
				device->setShader(pipeline_state, foundation::render::ShaderType::FRAGMENT, light.fragment_shader->shader);

				device->setBindSet(pipeline_state, light_binding, light.ibl_environment->bindings);

				device->clearVertexStreams(pipeline_state);
				device->setVertexStream(pipeline_state, 0, light.mesh->vertex_buffer);

				device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, light.mesh->index_buffer, light.mesh->num_indices);
			}
		}
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
		TextureHandle result = resource_manager->create<resources::Texture>();

		result->format = format;
		result->width = width;
		result->height = height;
		result->mip_levels = num_mips;
		result->layers = 1;
		result->gpu_data = device->createTexture2D(width, height, num_mips, format, data, num_data_mipmaps);

		managed_textures.push_back(result);

		return result;
	}

	TextureHandle APIImpl::createTexture2D(
		foundation::render::Format format,
		uint32_t width,
		uint32_t height,
		MeshHandle fullscreen_quad,
		ShaderHandle vertex_shader,
		ShaderHandle fragment_shader
	)
	{
		TextureHandle result = createTexture2D(format, width, height, 1);
		RenderUtils::renderTexture2D(device, result, fullscreen_quad, vertex_shader, fragment_shader);

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
		TextureHandle result = resource_manager->create<resources::Texture>();

		result->format = format;
		result->width = size;
		result->height = size;
		result->mip_levels = num_mips;
		result->layers = 6;
		result->gpu_data = device->createTextureCube(size, num_mips, format, data, num_data_mipmaps);

		managed_textures.push_back(result);

		return result;
	}

	TextureHandle APIImpl::loadTextureFromMemory(
		const uint8_t *data,
		size_t size
	)
	{
		TextureHandle result = resource_manager->importFromMemory<resources::Texture>(data, size, device);
		if (result.get())
			managed_textures.push_back(result);

		return result;
	}

	TextureHandle APIImpl::loadTexture(
		const foundation::io::URI &uri
	)
	{
		TextureHandle result = resource_manager->import<resources::Texture>(uri, device);

		// TODO: add resource to another lookup table in order not to load it twice
		// and also for future live reloading
		if (result.get())
			managed_textures.push_back(result);

		return result;
	}

	ShaderHandle APIImpl::loadShaderFromMemory(
		const uint8_t *data,
		size_t size,
		foundation::render::ShaderType shader_type
	)
	{
		ShaderHandle result = resource_manager->importFromMemory<resources::Shader>(data, size, shader_type, device, compiler);
		if (result.get())
			managed_shaders.push_back(result);

		return result;
	}

	ShaderHandle APIImpl::loadShader(
		const foundation::io::URI &uri,
		foundation::render::ShaderType shader_type
	)
	{
		ShaderHandle result = resource_manager->import<resources::Shader>(uri, shader_type, device, compiler);

		// TODO: add resource to another lookup table in order not to load it twice
		// and also for future live reloading
		if (result.get())
			managed_shaders.push_back(result);

		return result;
	}

	MeshHandle APIImpl::createMesh(
		uint32_t num_vertices,
		resources::Mesh::Vertex *vertices,
		uint32_t num_indices,
		uint32_t *indices
	)
	{
		MeshHandle result = resource_manager->create<resources::Mesh>();

		result->num_vertices = num_vertices;
		result->num_indices = num_indices;

		// TODO: use subresource pools
		result->vertices = new resources::Mesh::Vertex[result->num_vertices];
		result->indices = new uint32_t[result->num_indices];

		memcpy(result->vertices, vertices, sizeof(resources::Mesh::Vertex) * result->num_vertices);
		memcpy(result->indices, indices, sizeof(uint32_t) * result->num_indices);
		RenderUtils::uploadToGPU(device, result);

		managed_meshes.push_back(result);
		return result;
	}

	MeshHandle APIImpl::createMeshQuad(
		float size
	)
	{
		MeshHandle result = resource_manager->create<resources::Mesh>();

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
		RenderUtils::uploadToGPU(device, result);

		managed_meshes.push_back(result);
		return result;
	}

	MeshHandle APIImpl::createMeshSkybox(
		float size
	)
	{
		MeshHandle result = resource_manager->create<resources::Mesh>();

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
			0, 1, 2, 2, 3, 0,
			1, 5, 6, 6, 2, 1,
			3, 2, 6, 6, 7, 3,
			5, 4, 6, 4, 7, 6,
			1, 0, 4, 4, 5, 1,
			4, 0, 3, 3, 7, 4,
		};

		memcpy(result->indices, skybox_indices, sizeof(uint32_t) * result->num_indices);
		RenderUtils::uploadToGPU(device, result);

		managed_meshes.push_back(result);
		return result;
	}

	IBLTextureHandle APIImpl::importIBLTexture(
		const foundation::io::URI &uri,
		const IBLTextureCreateData &create_data
	)
	{
		TextureHandle equirectangular_texture = resource_manager->import<resources::Texture>(uri, device);
		if (equirectangular_texture.get() == nullptr)
			return IBLTextureHandle();

		uint32_t mips = static_cast<int>(std::floor(std::log2(create_data.cubemap_size)) + 1);

		TextureHandle diffuse_irradiance = resource_manager->create<resources::Texture>();
		diffuse_irradiance->format = create_data.format;
		diffuse_irradiance->width = create_data.cubemap_size;
		diffuse_irradiance->height = create_data.cubemap_size;
		diffuse_irradiance->mip_levels = 1;
		diffuse_irradiance->layers = 6;
		diffuse_irradiance->gpu_data = device->createTextureCube(create_data.cubemap_size, 1, create_data.format);

		TextureHandle temp_cubemap = resource_manager->create<resources::Texture>();
		temp_cubemap->format = create_data.format;
		temp_cubemap->width = create_data.cubemap_size;
		temp_cubemap->height = create_data.cubemap_size;
		temp_cubemap->mip_levels = 1;
		temp_cubemap->layers = 6;
		temp_cubemap->gpu_data = device->createTextureCube(create_data.cubemap_size, 1, create_data.format);

		TextureHandle prefiltered_specular = resource_manager->create<resources::Texture>();
		prefiltered_specular->format = create_data.format;
		prefiltered_specular->width = create_data.cubemap_size;
		prefiltered_specular->height = create_data.cubemap_size;
		prefiltered_specular->mip_levels = mips;
		prefiltered_specular->layers = 6;
		prefiltered_specular->gpu_data = device->createTextureCube(create_data.cubemap_size, mips, create_data.format);

		RenderUtils::renderHDRIToCube(
			device,
			prefiltered_specular,
			create_data.fullscreen_quad,
			create_data.cubemap_vertex,
			create_data.equirectangular_projection_fragment,
			create_data.prefiltered_specular_fragment,
			equirectangular_texture,
			temp_cubemap
		);

		RenderUtils::renderTextureCube(
			device,
			diffuse_irradiance,
			create_data.fullscreen_quad,
			create_data.cubemap_vertex,
			create_data.diffuse_irradiance_fragment,
			prefiltered_specular
		);

		resource_manager->destroy(equirectangular_texture, device);
		resource_manager->destroy(temp_cubemap, device);

		IBLTextureHandle result = resource_manager->create<resources::IBLTexture>();

		result->diffuse_irradiance_cubemap = diffuse_irradiance;
		result->prefiltered_specular_cubemap = prefiltered_specular;

		result->baked_brdf = create_data.baked_brdf;

		result->bindings = device->createBindSet();
		device->bindTexture(result->bindings, 0, result->baked_brdf->gpu_data);
		device->bindTexture(result->bindings, 1, result->prefiltered_specular_cubemap->gpu_data);
		device->bindTexture(result->bindings, 2, result->diffuse_irradiance_cubemap->gpu_data);

		managed_ibl_textures.push_back(result);
		return result;
	}

	RenderMaterialHandle APIImpl::createRenderMaterial(
		TextureHandle albedo,
		TextureHandle normal,
		TextureHandle roughness,
		TextureHandle metalness
	)
	{
		RenderMaterialHandle result = resource_manager->create<resources::RenderMaterial>();

		result->albedo = albedo;
		result->normal = normal;
		result->roughness = roughness;
		result->metalness = metalness;

		result->bindings = device->createBindSet();
		device->bindTexture(result->bindings, 0, result->albedo->gpu_data);
		device->bindTexture(result->bindings, 1, result->normal->gpu_data);
		device->bindTexture(result->bindings, 2, result->roughness->gpu_data);
		device->bindTexture(result->bindings, 3, result->metalness->gpu_data);

		managed_render_materials.push_back(result);
		return result;
	}
}
