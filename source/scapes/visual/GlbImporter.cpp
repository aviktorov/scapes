#include "GlbImporter.h"

#include <scapes/visual/API.h>
#include <scapes/visual/Resources.h>
#include <scapes/visual/Components.h>

#include <scapes/foundation/components/Components.h>
#include <scapes/foundation/game/World.h>
#include <scapes/foundation/game/Entity.h>
#include <scapes/foundation/math/Math.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <iostream>
#include <sstream>
#include <functional>
#include <map>

namespace scapes::visual
{
	/*
	 */
	GlbImporter *GlbImporter::create(
		foundation::resources::ResourceManager *resource_manager,
		foundation::game::World *world,
		foundation::render::Device *device
	)
	{
		return new impl::GlbImporter(resource_manager, world, device);
	}

	void GlbImporter::destroy(GlbImporter *importer)
	{
		delete importer;
	}
}

namespace scapes::visual::impl
{
	namespace cgltf
	{
		/*
		 */
		static foundation::math::vec3 toVec3(const cgltf_float transform[3])
		{
			return foundation::math::vec3(transform[0], transform[1], transform[2]);
		}

		static foundation::math::quat toQuat(const cgltf_float transform[4])
		{
			return foundation::math::quat(transform[3], transform[0], transform[1], transform[2]);
		}

		static foundation::math::mat4 toMat4(const cgltf_float transform[16])
		{
			return foundation::math::mat4(
				transform[0], transform[1], transform[2], transform[3],
				transform[4], transform[5], transform[6], transform[7],
				transform[8], transform[9], transform[10], transform[11],
				transform[12], transform[13], transform[14], transform[15]
			);
		}

		static foundation::math::mat4 getNodeTransform(const cgltf_node *node)
		{
			if (node->has_matrix)
				return toMat4(node->matrix);

			foundation::math::mat4 result = foundation::math::mat4(1.0f);

			if (node->has_translation)
				result = foundation::math::translate(foundation::math::mat4(1.0f), toVec3(node->translation));

			if (node->has_rotation)
				result = result * foundation::math::mat4(toQuat(node->rotation));

			if (node->has_scale)
				result = result * foundation::math::scale(foundation::math::mat4(1.0f), toVec3(node->scale));

			return result;
		};

		/*
		 */
		cgltf_result read(const cgltf_memory_options *memory_options, const cgltf_file_options *file_options, const char *path, cgltf_size *size, void **data)
		{
			foundation::io::FileSystem *file_system = reinterpret_cast<foundation::io::FileSystem *>(file_options->user_data);
			assert(file_system);

			size_t file_size = 0;
			void *file_data = file_system->map(path, file_size);

			if (!file_data)
			{
				foundation::Log::error("cgltf::read(): can't open \"%s\" file\n", path);
				return cgltf_result_file_not_found;
			}

			if (size)
				*size = static_cast<cgltf_size>(file_size);

			if (data)
				*data = file_data;

			return cgltf_result_success;
		}

		void release(const cgltf_memory_options *memory_options, const cgltf_file_options *file_options, void *data)
		{
			foundation::io::FileSystem *file_system = reinterpret_cast<foundation::io::FileSystem *>(file_options->user_data);
			assert(file_system);

			file_system->unmap(data);
		}
	}

	/*
	 */
	GlbImporter::GlbImporter(
		foundation::resources::ResourceManager *resource_manager,
		foundation::game::World *world,
		foundation::render::Device *device
	)
		: resource_manager(resource_manager), world(world), device(device)
	{
		assert(resource_manager);
		assert(world);
		assert(device);
	}

	GlbImporter::~GlbImporter()
	{

	}

	/*
	 */
	bool GlbImporter::import(const foundation::io::URI &uri, RenderMaterialHandle default_material)
	{
		cgltf_options parse_options = {};
		parse_options.file.read = cgltf::read;
		parse_options.file.release = cgltf::release;
		parse_options.file.user_data = resource_manager->getFileSystem();

		cgltf_data *data = nullptr;
		cgltf_result result = cgltf_parse_file(&parse_options, uri.c_str(), &data);
		if (result != cgltf_result_success)
		{
			std::cerr << "GlbImporter::import(): nismogla v parse" << std::endl;
			return false;
		}

		result = cgltf_load_buffers(&parse_options, data, uri.c_str());
		if (result != cgltf_result_success)
		{
			std::cerr << "GlbImporter::import(): nismogla v buffers" << std::endl;
			cgltf_free(data);
			return false;
		}

		// import meshes
		std::map<const cgltf_mesh *, visual::MeshHandle> mapped_meshes;

		for (cgltf_size i = 0; i < data->meshes_count; ++i)
		{
			visual::MeshHandle mesh = import_mesh(&data->meshes[i]);
			mapped_meshes.insert({&data->meshes[i], mesh});
		}

		// import images
		std::map<const cgltf_image *, visual::TextureHandle> mapped_textures;
		for (cgltf_size i = 0; i < data->images_count; ++i)
		{
			const cgltf_image &image = data->images[i];

			const uint8_t *data = cgltf_buffer_view_data(image.buffer_view);
			cgltf_size size = image.buffer_view->size;

			assert(data);
			assert(size);

			visual::TextureHandle texture = resource_manager->loadFromMemory<visual::resources::Texture>(data, size, device);

			mapped_textures.insert({&image, texture});
		}

		// import materials
		std::map<const cgltf_material *, visual::RenderMaterialHandle> mapped_materials;

		for (cgltf_size i = 0; i < data->materials_count; ++i)
		{
			const cgltf_material &material = data->materials[i];

			const cgltf_texture *base_color_texture = material.pbr_metallic_roughness.base_color_texture.texture;
			const cgltf_texture *normal_texture = material.normal_texture.texture;

			// TODO: metalness / roughness maps

			visual::TextureHandle albedo = default_material->albedo;
			visual::TextureHandle normal = default_material->normal;
			visual::TextureHandle roughness = default_material->roughness;
			visual::TextureHandle metalness = default_material->metalness;

			if (base_color_texture)
				albedo = mapped_textures[base_color_texture->image];

			if (normal_texture)
				normal = mapped_textures[normal_texture->image];

			visual::RenderMaterialHandle render_material = resource_manager->create<visual::resources::RenderMaterial>(
				albedo,
				normal,
				roughness,
				metalness,
				device
			);

			mapped_materials.insert({&material, render_material});
		}

		// import nodes
		std::function<void(const cgltf_node *, const foundation::math::mat4 &)> import_node;
		import_node = [&import_node, &mapped_materials, &mapped_meshes, this, default_material](const cgltf_node *node, const foundation::math::mat4 &transform) -> void
		{
			if (node->mesh)
			{
				auto it = mapped_meshes.find(node->mesh);
				assert(it != mapped_meshes.end());

				visual::MeshHandle mesh = it->second;

				foundation::game::Entity entity = foundation::game::Entity(world);

				auto mat_it = mapped_materials.find(node->mesh->primitives[0].material);

				visual::RenderMaterialHandle material = (mat_it != mapped_materials.end()) ? mat_it->second : default_material;

				entity.addComponent<foundation::components::Transform>(transform);
				entity.addComponent<visual::components::Renderable>(mesh, material);
			}

			for (cgltf_size i = 0; i < node->children_count; ++i)
				import_node(node->children[i], transform * cgltf::getNodeTransform(node->children[i]));
		};

		for (cgltf_size i = 0; i < data->nodes_count; ++i)
		{
			const cgltf_node &node = data->nodes[i];
			if (node.parent != nullptr)
				continue;

			import_node(&node, cgltf::getNodeTransform(&node));
		}

		cgltf_free(data);

		return true;
	}

	/*
	 */
	visual::MeshHandle GlbImporter::import_mesh(const cgltf_mesh *mesh)
	{
		assert(mesh);
		assert(mesh->primitives_count >= 1);

		if (mesh->primitives_count > 1)
			foundation::Log::warning("GlbImporter::import_mesh(): mesh contains more than one primitive, current importer does not support submeshes\n");

		const cgltf_primitive &primitive = mesh->primitives[0];

		const cgltf_accessor *cgltf_positions = nullptr;
		const cgltf_accessor *cgltf_tangets = nullptr;
		const cgltf_accessor *cgltf_normals = nullptr;
		const cgltf_accessor *cgltf_uv = nullptr;
		const cgltf_accessor *cgltf_colors = nullptr;
		const cgltf_accessor *cgltf_indices = primitive.indices;

		for (cgltf_size i = 0; i < primitive.attributes_count; ++i)
		{
			const cgltf_attribute &attribute = primitive.attributes[i];
			switch (attribute.type)
			{
				case cgltf_attribute_type_position: cgltf_positions = attribute.data; break;
				case cgltf_attribute_type_normal: cgltf_normals = attribute.data; break;
				case cgltf_attribute_type_tangent: cgltf_tangets = attribute.data; break;
				case cgltf_attribute_type_texcoord: cgltf_uv = attribute.data; break;
				case cgltf_attribute_type_color: cgltf_colors = attribute.data; break;
			}
		}

		assert(cgltf_indices && cgltf_positions && cgltf_normals);
		assert(cgltf_positions->count == cgltf_normals->count);

		visual::resources::Mesh::Vertex *vertices = nullptr;
		uint32_t *indices = nullptr;

		uint32_t num_vertices = static_cast<uint32_t>(cgltf_positions->count);
		uint32_t num_indices = static_cast<uint32_t>(cgltf_indices->count);

		vertices = new visual::resources::Mesh::Vertex[num_vertices];
		indices = new uint32_t[num_indices];

		memset(vertices, 0, sizeof(visual::resources::Mesh::Vertex) * num_vertices);
		memset(indices, 0, sizeof(uint32_t) * num_indices);

		for (cgltf_size i = 0; i < cgltf_positions->count; i++)
		{
			cgltf_bool success = cgltf_accessor_read_float(cgltf_positions, i, (cgltf_float*)&vertices[i].position, 3);
			assert(success);
		}

		if (cgltf_tangets)
		{
			assert(cgltf_positions->count == cgltf_tangets->count);
			for (cgltf_size i = 0; i < cgltf_positions->count; i++)
			{
				cgltf_bool success = cgltf_accessor_read_float(cgltf_tangets, i, (cgltf_float*)&vertices[i].tangent, 4);
				assert(success);
			}
		}

		for (cgltf_size i = 0; i < cgltf_positions->count; i++)
		{
			cgltf_bool success = cgltf_accessor_read_float(cgltf_normals, i, (cgltf_float*)&vertices[i].normal, 3);
			assert(success);
		}

		for (cgltf_size i = 0; i < cgltf_positions->count; i++)
			vertices[i].binormal = foundation::math::cross(vertices[i].normal, foundation::math::vec3(vertices[i].tangent));

		if (cgltf_uv)
		{
			assert(cgltf_positions->count == cgltf_uv->count);
			for (cgltf_size i = 0; i < cgltf_positions->count; i++)
			{
				cgltf_bool success = cgltf_accessor_read_float(cgltf_uv, i, (cgltf_float*)&vertices[i].uv, 2);
				assert(success);
			}
		}

		if (cgltf_colors)
		{
			assert(cgltf_positions->count == cgltf_colors->count);
			for (cgltf_size i = 0; i < cgltf_positions->count; i++)
			{
				cgltf_bool success = cgltf_accessor_read_float(cgltf_colors, i, (cgltf_float*)&vertices[i].color, 4);
				assert(success);
			}
		}

		for (cgltf_size i = 0; i < cgltf_indices->count; i++)
		{
			cgltf_bool success = cgltf_accessor_read_uint(cgltf_indices, i, &indices[i], 1);
			assert(success);
		}

		visual::MeshHandle result = resource_manager->create<resources::Mesh>(device, num_vertices, vertices, num_indices, indices);

		delete[] vertices;
		delete[] indices;

		return result;
	}
}
