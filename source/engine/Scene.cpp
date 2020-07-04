#include "Scene.h"

#include "VulkanMesh.h"
#include "VulkanTexture.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <sstream>

using namespace render::backend;

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

static Texture *default_albedo = nullptr;
static Texture *default_normal = nullptr;
static Texture *default_roughness = nullptr;
static Texture *default_metalness = nullptr;

static void generateDefaultTextures(Driver *driver)
{
	auto generate_texture = [=](uint8_t r, uint8_t g, uint8_t b) -> Texture *
	{
		uint8_t pixels[16] = {
			r, g, b, 255,
			r, g, b, 255,
			r, g, b, 255,
			r, g, b, 255,
		};

		return driver->createTexture2D(2, 2, 1, Format::R8G8B8A8_UNORM, Multisample::COUNT_1, pixels);
	};

	if (default_albedo == nullptr)
		default_albedo = generate_texture(127, 127, 127);

	if (default_normal == nullptr)
		default_normal = generate_texture(127, 127, 255);

	if (default_roughness == nullptr)
		default_roughness = generate_texture(255, 255, 255);

	if (default_metalness == nullptr)
		default_metalness = generate_texture(0, 0, 0);
}

/*
 */
Scene::~Scene()
{
	clear();
}

/*
 */
bool Scene::import(const char *path)
{
	generateDefaultTextures(driver);

	Assimp::Importer importer;

	const aiScene *scene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_MaxQuality);

	if (!scene)
	{
		std::cerr << "Scene::import(): " << importer.GetErrorString() << std::endl;
		return false;
	}

	if (!scene->HasMeshes())
	{
		std::cerr << "Scene::import(): model has no meshes" << std::endl;
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
	meshes.resize(scene->mNumMeshes);
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		VulkanMesh *mesh = new VulkanMesh(driver);
		mesh->import(scene->mMeshes[i]);

		meshes[i] = mesh;
	}

	// import textures
	for (unsigned int i = 0; i < scene->mNumTextures; ++i)
	{
		std::stringstream path_builder;
		VulkanTexture *texture = new VulkanTexture(driver);
		
		path_builder << dir << '/' << scene->mTextures[i]->mFilename.C_Str();
		const std::string &texture_path = path_builder.str();
		texture->import(texture_path.c_str());

		textures.insert({texture_path, texture});
	}

	// import materials
	materials.resize(scene->mNumMaterials);

	auto import_material_texture = [=](const aiMaterial *material, aiTextureType type) -> VulkanTexture *
	{
		aiString path;
		material->GetTexture(type, 0, &path);

		if (path.length == 0)
			return nullptr;

		std::stringstream path_builder;
		path_builder << dir << '/' << path.C_Str();

		const std::string &texture_path = path_builder.str();
		auto it = textures.find(texture_path);
		if (it == textures.end())
		{
			VulkanTexture *texture = new VulkanTexture(driver);
			texture->import(texture_path.c_str());

			textures.insert({texture_path, texture});
			return texture;
		}

		return it->second;
	};

	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		const aiMaterial *material = scene->mMaterials[i];

		RenderMaterial &render_material = materials[i];
		render_material.albedo = import_material_texture(material, aiTextureType_DIFFUSE);
		render_material.normal = import_material_texture(material, aiTextureType_HEIGHT);
		render_material.roughness = import_material_texture(material, aiTextureType_SHININESS);
		render_material.metalness = import_material_texture(material, aiTextureType_AMBIENT);

		render_material.bindings = driver->createBindSet();

		const Texture *albedo = (render_material.albedo) ? render_material.albedo->getBackend() : default_albedo;
		const Texture *normal = (render_material.normal) ? render_material.normal->getBackend() : default_normal;
		const Texture *roughness = (render_material.roughness) ? render_material.roughness->getBackend() : default_roughness;
		const Texture *metalness = (render_material.metalness) ? render_material.metalness->getBackend() : default_metalness;

		driver->bindTexture(render_material.bindings, 0, albedo);
		driver->bindTexture(render_material.bindings, 1, normal);
		driver->bindTexture(render_material.bindings, 2, roughness);
		driver->bindTexture(render_material.bindings, 3, metalness);
	}

	// import nodes
	const aiNode * root = scene->mRootNode;
	importNodes(scene, root, toGlm(root->mTransformation));

	return true;
}

void Scene::clear()
{
	for (size_t i = 0; i < meshes.size(); ++i)
		delete meshes[i];

	for (size_t i = 0; i < materials.size(); ++i)
		driver->destroyBindSet(materials[i].bindings);

	for (auto it = textures.begin(); it != textures.end(); ++it)
		delete it->second;

	meshes.clear();
	materials.clear();
	textures.clear();
	nodes.clear();
}

/*
 */
void Scene::importNodes(const aiScene *scene, const aiNode *root, const glm::mat4 &transform)
{
	for (unsigned int i = 0; i < root->mNumMeshes; ++i)
	{
		unsigned int mesh_index = root->mMeshes[i];
		int32_t material_index = static_cast<int32_t>(scene->mMeshes[mesh_index]->mMaterialIndex);

		VulkanMesh *mesh = meshes[mesh_index];

		RenderNode node;
		node.mesh = mesh;
		node.transform = transform;
		node.render_material_index = material_index;

		nodes.push_back(node);
	}

	for (unsigned int i = 0; i < root->mNumChildren; ++i)
	{
		const aiNode *child = root->mChildren[i];
		const glm::mat4 &child_transform = transform * toGlm(child->mTransformation);

		importNodes(scene, child, child_transform);
	}
}
