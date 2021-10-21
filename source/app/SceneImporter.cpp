#include "SceneImporter.h"
#include "ApplicationResources.h"

#include <scapes/visual/API.h>
#include <scapes/visual/Resources.h>
#include <scapes/visual/Components.h>

#include <game/World.h>

#include <common/Math.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <iostream>
#include <sstream>
#include <functional>
#include <map>

/*
 */
static glm::mat4 toGlm(const aiMatrix4x4 &transform)
{
	glm::mat4 result;

	result[0].x = transform.a1; result[1].x = transform.a2; result[2].x = transform.a3; result[3].x = transform.a4;
	result[0].y = transform.b1; result[1].y = transform.b2; result[2].y = transform.b3; result[3].y = transform.b4;
	result[0].z = transform.c1; result[1].z = transform.c2; result[2].z = transform.c3; result[3].z = transform.c4;
	result[0].w = transform.d1; result[1].w = transform.d2; result[2].w = transform.d3; result[3].w = transform.d4;

	return result;
}

static glm::vec3 toGlmVec3(const cgltf_float transform[3])
{
	return glm::vec3(transform[0], transform[1], transform[2]);
}

static glm::quat toGlmQuat(const cgltf_float transform[4])
{
	return glm::quat(transform[3], transform[0], transform[1], transform[2]);
}

static glm::mat4 toGlmMat4(const cgltf_float transform[16])
{
	return glm::mat4(
		transform[0], transform[1], transform[2], transform[3],
		transform[4], transform[5], transform[6], transform[7],
		transform[8], transform[9], transform[10], transform[11],
		transform[12], transform[13], transform[14], transform[15]
	);
}

static glm::mat4 getNodeTransform(const cgltf_node *node)
{
	if (node->has_matrix)
		return toGlmMat4(node->matrix);

	glm::mat4 result = glm::mat4(1.0f);

	if (node->has_translation)
		result = glm::translate(glm::mat4(1.0f), toGlmVec3(node->translation));

	if (node->has_rotation)
		result = result * glm::mat4(toGlmQuat(node->rotation));

	if (node->has_scale)
		result = result * glm::scale(glm::mat4(1.0f), toGlmVec3(node->scale));

	return result;
};

/*
 */
SceneImporter::SceneImporter(game::World *world, scapes::visual::API *visual_api)
	: world(world), visual_api(visual_api)
{
}

SceneImporter::~SceneImporter()
{
	clear();
}

/*
 */
bool SceneImporter::importCGLTF(const char *path, ApplicationResources *resources)
{
	cgltf_options options = {};
	cgltf_data *data = nullptr;
	cgltf_result result = cgltf_parse_file(&options, path, &data);
	if (result != cgltf_result_success)
	{
		std::cerr << "SceneImporter::import(): nismogla v parse" << std::endl;
		return false;
	}

	result = cgltf_load_buffers(&options, data, path);
	if (result != cgltf_result_success)
	{
		std::cerr << "SceneImporter::import(): nismogla v buffers" << std::endl;
		cgltf_free(data);
		return false;
	}

	// import meshes
	std::map<const cgltf_mesh*, scapes::visual::MeshHandle > mapped_meshes;

	for (cgltf_size i = 0; i < data->meshes_count; ++i)
	{
		scapes::visual::MeshHandle mesh = import_cgltf_mesh(&data->meshes[i]);
		mapped_meshes.insert({&data->meshes[i], mesh});
	}

	// import images
	std::map<const cgltf_image *, scapes::visual::TextureHandle > mapped_textures;
	for (cgltf_size i = 0; i < data->images_count; ++i)
	{
		const cgltf_image &image = data->images[i];

		const uint8_t *data = cgltf_buffer_view_data(image.buffer_view);
		cgltf_size size = image.buffer_view->size;

		assert(data);
		assert(size);

		scapes::visual::TextureHandle texture = visual_api->loadTextureFromMemory(data, size);

		mapped_textures.insert({&image, texture});
	}


	// import materials
	std::map<const cgltf_material*, scapes::visual::RenderMaterialHandle > mapped_materials;

	scapes::visual::RenderMaterialHandle default_material = visual_api->createRenderMaterial(
		resources->getDefaultAlbedo(),
		resources->getDefaultNormal(),
		resources->getDefaultRoughness(),
		resources->getDefaultMetalness()
	);

	for (cgltf_size i = 0; i < data->materials_count; ++i)
	{
		const cgltf_material &material = data->materials[i];

		const cgltf_texture *base_color_texture = material.pbr_metallic_roughness.base_color_texture.texture;
		const cgltf_texture *normal_texture = material.normal_texture.texture;

		// TODO: metalness / roughness maps

		scapes::visual::TextureHandle albedo = resources->getDefaultAlbedo();
		scapes::visual::TextureHandle normal = resources->getDefaultNormal();
		scapes::visual::TextureHandle roughness = resources->getDefaultRoughness();
		scapes::visual::TextureHandle metalness = resources->getDefaultMetalness();

		if (base_color_texture)
			albedo = mapped_textures[base_color_texture->image];

		if (normal_texture)
			normal = mapped_textures[normal_texture->image];

		scapes::visual::RenderMaterialHandle render_material = visual_api->createRenderMaterial(albedo, normal, roughness, metalness);

		mapped_materials.insert({&material, render_material});
	}

	// import nodes
	std::function<void(const cgltf_node*, const glm::mat4&)> import_node;
	import_node = [&import_node, &mapped_materials, &mapped_meshes, this, default_material](const cgltf_node *node, const glm::mat4 &transform) -> void
	{
		if (node->mesh)
		{
			auto it = mapped_meshes.find(node->mesh);
			assert(it != mapped_meshes.end());

			scapes::visual::MeshHandle mesh = it->second;

			game::Entity entity = game::Entity(world);

			auto mat_it = mapped_materials.find(node->mesh->primitives[0].material);

			scapes::visual::RenderMaterialHandle material = (mat_it != mapped_materials.end()) ? mat_it->second : default_material;

			entity.addComponent<scapes::visual::components::Transform>(transform);
			entity.addComponent<scapes::visual::components::Renderable>(mesh, material);
		}

		for (cgltf_size i = 0; node->children_count; ++i)
			import_node(node->children[i], transform * getNodeTransform(node->children[i]));
	};

	for (cgltf_size i = 0; i < data->nodes_count; ++i)
	{
		const cgltf_node &node = data->nodes[i];
		if (node.parent != nullptr)
			continue;

		import_node(&node, getNodeTransform(&node));
	}

	cgltf_free(data);

	return true;
}

bool SceneImporter::importAssimp(const char *path, ApplicationResources *resources)
{
	Assimp::Importer importer;

	const aiScene *scene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_MaxQuality);

	if (!scene)
	{
		std::cerr << "SceneImporter::import(): " << importer.GetErrorString() << std::endl;
		return false;
	}

	if (!scene->HasMeshes())
	{
		std::cerr << "SceneImporter::import(): model has no meshes" << std::endl;
		return false;
	}

	clear();

	const char *end = strrchr(path, '/');
	if (end == nullptr)
		end = strrchr(path, '\\');

	std::string dir;

	if (end != nullptr)
		dir = std::string(path, strlen(path) - strlen(end));
	
	// import meshes
	std::vector<scapes::visual::MeshHandle > imported_meshes;
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		scapes::visual::MeshHandle mesh = import_assimp_mesh(scene->mMeshes[i]);
		imported_meshes.push_back(mesh);
	}

	// import textures
	std::map<std::string, scapes::visual::TextureHandle > mapped_textures;
	for (unsigned int i = 0; i < scene->mNumTextures; ++i)
	{
		// TODO: fetch from resource manager
		std::stringstream path_builder;
		
		path_builder << dir << '/' << scene->mTextures[i]->mFilename.C_Str();
		const std::string &texture_path = path_builder.str();

		scapes::visual::TextureHandle texture = visual_api->loadTexture(texture_path.c_str());

		mapped_textures.insert({texture_path, texture});
	}

	// import materials
	auto import_material_texture = [&mapped_textures, &dir, this, resources](
		const aiMaterial *material,
		aiTextureType type,
		scapes::visual::TextureHandle default_texture
	) -> scapes::visual::TextureHandle
	{
		aiString path;
		material->GetTexture(type, 0, &path);

		if (path.length == 0)
			return default_texture;

		std::stringstream path_builder;
		path_builder << dir << '/' << path.C_Str();

		// TODO: fetch from resource manager
		const std::string &texture_path = path_builder.str();
		auto it = mapped_textures.find(texture_path);
		if (it == mapped_textures.end())
		{
			scapes::visual::TextureHandle texture = visual_api->loadTexture(texture_path.c_str());

			mapped_textures.insert({texture_path, texture});
			return texture;
		}

		return it->second;
	};

	std::vector<scapes::visual::RenderMaterialHandle > imported_materials;
	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		const aiMaterial *material = scene->mMaterials[i];

		scapes::visual::TextureHandle albedo = import_material_texture(material, aiTextureType_DIFFUSE, resources->getDefaultAlbedo());
		scapes::visual::TextureHandle normal = import_material_texture(material, aiTextureType_HEIGHT, resources->getDefaultNormal());
		scapes::visual::TextureHandle roughness = import_material_texture(material, aiTextureType_SHININESS, resources->getDefaultRoughness());
		scapes::visual::TextureHandle metalness = import_material_texture(material, aiTextureType_AMBIENT, resources->getDefaultMetalness());

		imported_materials.push_back(visual_api->createRenderMaterial(albedo, normal, roughness, metalness));
	}

	// import nodes
	std::function<void(const aiScene*, const aiNode*, const aiMatrix4x4&)> import_node;
	import_node = [&import_node, &imported_meshes, &imported_materials, this](const aiScene *scene, const aiNode *root, const aiMatrix4x4 &transform) -> void
	{
		for (unsigned int i = 0; i < root->mNumMeshes; ++i)
		{
			unsigned int mesh_index = root->mMeshes[i];
			int32_t material_index = static_cast<int32_t>(scene->mMeshes[mesh_index]->mMaterialIndex);

			game::Entity entity = game::Entity(world);

			entity.addComponent<scapes::visual::components::Transform>(toGlm(transform));
			entity.addComponent<scapes::visual::components::Renderable>(imported_meshes[mesh_index], imported_materials[material_index]);
		}

		for (unsigned int i = 0; i < root->mNumChildren; ++i)
		{
			const aiNode *child = root->mChildren[i];
			const aiMatrix4x4 &child_transform = transform * child->mTransformation;

			import_node(scene, child, child_transform);
		}
	};

	// TODO: remove later
	aiMatrix4x4 rotation = aiMatrix4x4::RotationX(AI_DEG_TO_RAD(90.0f), aiMatrix4x4());

	const aiNode *root = scene->mRootNode;
	import_node(scene, root, root->mTransformation * rotation);

	return true;
}

void SceneImporter::clear()
{
	world->clear();
}

/*
 */
scapes::visual::MeshHandle SceneImporter::import_cgltf_mesh(const cgltf_mesh *mesh)
{
	assert(mesh);
	assert(mesh->primitives_count == 1);

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

	scapes::visual::resources::Mesh::Vertex *vertices = nullptr;
	uint32_t *indices = nullptr;

	uint32_t num_vertices = static_cast<uint32_t>(cgltf_positions->count);
	uint32_t num_indices = static_cast<uint32_t>(cgltf_indices->count);

	// TODO: use subresource pools
	vertices = new scapes::visual::resources::Mesh::Vertex[num_vertices];
	indices = new uint32_t[num_indices];

	memset(vertices, 0, sizeof(scapes::visual::resources::Mesh::Vertex) * num_vertices);
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
		vertices[i].binormal = glm::cross(vertices[i].normal, glm::vec3(vertices[i].tangent));

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

	scapes::visual::MeshHandle result = visual_api->createMesh(num_vertices, vertices, num_indices, indices);

	delete[] vertices;
	delete[] indices;

	return result;
}

scapes::visual::MeshHandle SceneImporter::import_assimp_mesh(const aiMesh *mesh)
{
	assert(mesh != nullptr);

	scapes::visual::resources::Mesh::Vertex *vertices = nullptr;
	uint32_t *indices;

	uint32_t num_vertices = mesh->mNumVertices;
	uint32_t num_indices = mesh->mNumFaces * 3;

	// TODO: use subresource pools
	vertices = new scapes::visual::resources::Mesh::Vertex[num_vertices];
	indices = new uint32_t[num_indices];

	aiVector3D *meshVertices = mesh->mVertices;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		vertices[i].position = glm::vec3(meshVertices[i].x, meshVertices[i].y, meshVertices[i].z);

	aiVector3D *meshTangents = mesh->mTangents;
	if (meshTangents)
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			vertices[i].tangent = glm::vec4(meshTangents[i].x, meshTangents[i].y, meshTangents[i].z, 0.0f);
	else
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			vertices[i].tangent = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

	aiVector3D *meshBinormals = mesh->mBitangents;
	if (meshBinormals)
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			vertices[i].binormal = glm::vec3(meshBinormals[i].x, meshBinormals[i].y, meshBinormals[i].z);
	else
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			vertices[i].binormal = glm::vec3(0.0f, 0.0f, 0.0f);

	aiVector3D *meshNormals = mesh->mNormals;
	if (meshNormals)
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			vertices[i].normal = glm::vec3(meshNormals[i].x, meshNormals[i].y, meshNormals[i].z);
	else
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			vertices[i].normal = glm::vec3(0.0f, 0.0f, 0.0f);

	aiVector3D *meshUVs = mesh->mTextureCoords[0];
	if (meshUVs)
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			vertices[i].uv = glm::vec2(meshUVs[i].x, 1.0f - meshUVs[i].y);
	else
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			vertices[i].uv = glm::vec2(0.0f, 0.0f);

	aiColor4D *meshColors = mesh->mColors[0];
	if (meshColors)
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			vertices[i].color = glm::vec4(meshColors[i].r, meshColors[i].g, meshColors[i].b, meshColors[i].a);
	else
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			vertices[i].color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	aiFace *meshFaces = mesh->mFaces;
	unsigned int index = 0;
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		for (unsigned int faceIndex = 0; faceIndex < meshFaces[i].mNumIndices; faceIndex++)
			indices[index++] = meshFaces[i].mIndices[faceIndex];

	scapes::visual::MeshHandle result = visual_api->createMesh(num_vertices, vertices, num_indices, indices);

	delete[] vertices;
	delete[] indices;

	return result;
}
