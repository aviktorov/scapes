#include "SceneImporter.h"
#include "ApplicationResources.h"

#include "Mesh.h"
#include "Texture.h"
#include "RenderModule.h"

#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <render/backend/Driver.h>
#include <game/World.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>

#include <iostream>
#include <sstream>
#include <functional>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace resources;

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
SceneImporter::SceneImporter(render::backend::Driver *driver, game::World *world)
	: driver(driver), world(world)
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
	std::map<const cgltf_mesh*, ResourceHandle<Mesh> > mapped_meshes;

	for (cgltf_size i = 0; i < data->meshes_count; ++i)
	{
		ResourceHandle<Mesh> mesh = resources->createMeshFromCGLTF(&data->meshes[i]);
		mapped_meshes.insert({&data->meshes[i], mesh});
	}

	// import images
	std::map<const cgltf_image *, ResourceHandle<Texture> > mapped_textures;
	for (cgltf_size i = 0; i < data->images_count; ++i)
	{
		const cgltf_image &image = data->images[i];

		const uint8_t *data = cgltf_buffer_view_data(image.buffer_view);
		cgltf_size size = image.buffer_view->size;

		assert(data);
		assert(size);
		
		// TODO: fetch from resource manager
		ResourceHandle<Texture> texture = resources->loadTextureFromMemory(data, size);

		mapped_textures.insert({&image, texture});
	}


	// import materials
	std::map<const cgltf_material*, ResourceHandle<RenderMaterial> > mapped_materials;

	ResourceHandle<RenderMaterial> default_material = resources->createRenderMaterial(
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

		ResourceHandle<Texture> albedo = resources->getDefaultAlbedo();
		ResourceHandle<Texture> normal = resources->getDefaultNormal();
		ResourceHandle<Texture> roughness = resources->getDefaultRoughness();
		ResourceHandle<Texture> metalness = resources->getDefaultMetalness();

		if (base_color_texture)
			albedo = mapped_textures[base_color_texture->image];

		if (normal_texture)
			normal = mapped_textures[normal_texture->image];

		ResourceHandle<RenderMaterial> render_material = resources->createRenderMaterial(albedo, normal, roughness, metalness);

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

			ResourceHandle<Mesh> mesh = it->second;

			game::Entity entity = game::Entity(world);

			auto mat_it = mapped_materials.find(node->mesh->primitives[0].material);

			ResourceHandle<RenderMaterial> material = (mat_it != mapped_materials.end()) ? mat_it->second : default_material;

			entity.addComponent<ecs::render::Transform>(transform);
			entity.addComponent<ecs::render::Renderable>(mesh, material);
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
	std::vector<ResourceHandle<Mesh> > imported_meshes;
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		ResourceHandle<Mesh> mesh = resources->createMeshFromAssimp(scene->mMeshes[i]);
		imported_meshes.push_back(mesh);
	}

	// import textures
	std::map<std::string, ResourceHandle<Texture> > mapped_textures;
	for (unsigned int i = 0; i < scene->mNumTextures; ++i)
	{
		// TODO: fetch from resource manager
		std::stringstream path_builder;
		
		path_builder << dir << '/' << scene->mTextures[i]->mFilename.C_Str();
		const std::string &texture_path = path_builder.str();

		ResourceHandle<Texture> texture = resources->loadTexture(texture_path.c_str());

		mapped_textures.insert({texture_path, texture});
	}

	// import materials
	auto import_material_texture = [&mapped_textures, &dir, this, resources](
		const aiMaterial *material,
		aiTextureType type,
		ResourceHandle<Texture> default_texture
	) -> ResourceHandle<Texture>
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
			ResourceHandle<Texture> texture = resources->loadTexture(texture_path.c_str());

			mapped_textures.insert({texture_path, texture});
			return texture;
		}

		return it->second;
	};

	std::vector<ResourceHandle<RenderMaterial> > imported_materials;
	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		const aiMaterial *material = scene->mMaterials[i];

		ResourceHandle<Texture> albedo = import_material_texture(material, aiTextureType_DIFFUSE, resources->getDefaultAlbedo());
		ResourceHandle<Texture> normal = import_material_texture(material, aiTextureType_HEIGHT, resources->getDefaultNormal());
		ResourceHandle<Texture> roughness = import_material_texture(material, aiTextureType_SHININESS, resources->getDefaultRoughness());
		ResourceHandle<Texture> metalness = import_material_texture(material, aiTextureType_AMBIENT, resources->getDefaultMetalness());

		imported_materials.push_back(resources->createRenderMaterial(albedo, normal, roughness, metalness));
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

			entity.addComponent<ecs::render::Transform>(toGlm(transform));
			entity.addComponent<ecs::render::Renderable>(imported_meshes[mesh_index], imported_materials[material_index]);
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
