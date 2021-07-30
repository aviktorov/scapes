#include "Scene.h"

#include "Mesh.h"
#include "Texture.h"
#include "RenderModule.h"

#include <glm/mat4x4.hpp>

#include <render/backend/Driver.h>
#include <game/World.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <sstream>

/*
 */
static render::backend::Texture *default_albedo = nullptr;
static render::backend::Texture *default_normal = nullptr;
static render::backend::Texture *default_roughness = nullptr;
static render::backend::Texture *default_metalness = nullptr;

/*
 */
static render::backend::Texture *generateTexture(render::backend::Driver *driver, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t pixels[16] = {
		r, g, b, 255,
		r, g, b, 255,
		r, g, b, 255,
		r, g, b, 255,
	};

	return driver->createTexture2D(2, 2, 1, render::backend::Format::R8G8B8A8_UNORM, pixels);
}

static void generateDefaultTextures(render::backend::Driver *driver)
{
	if (default_albedo == nullptr)
		default_albedo = generateTexture(driver, 127, 127, 127);

	if (default_normal == nullptr)
		default_normal = generateTexture(driver, 127, 127, 255);

	if (default_roughness == nullptr)
		default_roughness = generateTexture(driver, 255, 255, 255);

	if (default_metalness == nullptr)
		default_metalness = generateTexture(driver, 0, 0, 0);
}

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

/*
 */
Scene::Scene(render::backend::Driver *driver, game::World *world)
	: driver(driver), world(world)
{
	generateDefaultTextures(driver);
}

Scene::~Scene()
{
	clear();
}

/*
 */
bool Scene::import(const char *path)
{
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
		// TODO: fetch from resource manager
		Mesh *mesh = new Mesh(driver);
		mesh->import(scene->mMeshes[i]);

		meshes[i] = mesh;
	}

	// import textures
	for (unsigned int i = 0; i < scene->mNumTextures; ++i)
	{
		// TODO: fetch from resource manager
		std::stringstream path_builder;
		Texture *texture = new Texture(driver);
		
		path_builder << dir << '/' << scene->mTextures[i]->mFilename.C_Str();
		const std::string &texture_path = path_builder.str();
		texture->import(texture_path.c_str());

		textures.insert({texture_path, texture});
	}

	// import materials
	materials.resize(scene->mNumMaterials);

	auto import_material_texture = [=](const aiMaterial *material, aiTextureType type) -> Texture *
	{
		aiString path;
		material->GetTexture(type, 0, &path);

		if (path.length == 0)
			return nullptr;

		std::stringstream path_builder;
		path_builder << dir << '/' << path.C_Str();

		// TODO: fetch from resource manager
		const std::string &texture_path = path_builder.str();
		auto it = textures.find(texture_path);
		if (it == textures.end())
		{
			Texture *texture = new Texture(driver);
			texture->import(texture_path.c_str());

			textures.insert({texture_path, texture});
			return texture;
		}

		return it->second;
	};

	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		const aiMaterial *material = scene->mMaterials[i];

		ecs::render::RenderMaterialData *render_material = new ecs::render::RenderMaterialData();

		render_material->albedo = import_material_texture(material, aiTextureType_DIFFUSE);
		render_material->normal = import_material_texture(material, aiTextureType_HEIGHT);
		render_material->roughness = import_material_texture(material, aiTextureType_SHININESS);
		render_material->metalness = import_material_texture(material, aiTextureType_AMBIENT);

		render_material->bindings = driver->createBindSet();
		render_material->parameters = nullptr;

		const render::backend::Texture *albedo = (render_material->albedo) ? render_material->albedo->getBackend() : default_albedo;
		const render::backend::Texture *normal = (render_material->normal) ? render_material->normal->getBackend() : default_normal;
		const render::backend::Texture *roughness = (render_material->roughness) ? render_material->roughness->getBackend() : default_roughness;
		const render::backend::Texture *metalness = (render_material->metalness) ? render_material->metalness->getBackend() : default_metalness;

		driver->bindTexture(render_material->bindings, 0, albedo);
		driver->bindTexture(render_material->bindings, 1, normal);
		driver->bindTexture(render_material->bindings, 2, roughness);
		driver->bindTexture(render_material->bindings, 3, metalness);

		materials[i] = render_material;
	}

	// import nodes
	const aiNode * root = scene->mRootNode;

	// TODO: remove later
	aiMatrix4x4 rotation = aiMatrix4x4::RotationX(AI_DEG_TO_RAD(90.0f), aiMatrix4x4());

	importNodes(scene, root, root->mTransformation * rotation);

	return true;
}

void Scene::clear()
{
	world->clear();

	// TODO: move to resource manager
	for (size_t i = 0; i < materials.size(); ++i)
	{
		ecs::render::RenderMaterialData *material = materials[i];

		driver->destroyBindSet(material->bindings);
		driver->destroyUniformBuffer(material->parameters);
		delete material;
	}

	for (size_t i = 0; i < environment_textures.size(); ++i)
	{
		ecs::render::EnvironmentTexture *environment_texture = environment_textures[i];

		driver->destroyBindSet(environment_texture->bindings);
		delete environment_texture;
	}

	materials.clear();
	environment_textures.clear();

	for (size_t i = 0; i < meshes.size(); ++i)
		delete meshes[i];

	for (auto it = textures.begin(); it != textures.end(); ++it)
		delete it->second;

	meshes.clear();
	textures.clear();
}

/*
 */
const ecs::render::EnvironmentTexture *Scene::fetchEnvironmentTexture(
	const Texture *baked_brdf,
	const Texture *prefiltered_specular_cubemap,
	const Texture *diffuse_irradiance_cubemap
)
{
	// TODO: don't allocate every time the same resource was requested
	ecs::render::EnvironmentTexture *environment_texture = new ecs::render::EnvironmentTexture();

	environment_texture->baked_brdf = baked_brdf;
	environment_texture->prefiltered_specular_cubemap = prefiltered_specular_cubemap;
	environment_texture->diffuse_irradiance_cubemap = diffuse_irradiance_cubemap;
	environment_texture->bindings = driver->createBindSet();

	driver->bindTexture(environment_texture->bindings, 0, baked_brdf->getBackend());
	driver->bindTexture(environment_texture->bindings, 1, prefiltered_specular_cubemap->getBackend());
	driver->bindTexture(environment_texture->bindings, 2, diffuse_irradiance_cubemap->getBackend());

	environment_textures.push_back(environment_texture);
	return environment_texture;
}

/*
 */
void Scene::importNodes(const aiScene *scene, const aiNode *root, const aiMatrix4x4 &transform)
{
	for (unsigned int i = 0; i < root->mNumMeshes; ++i)
	{
		unsigned int mesh_index = root->mMeshes[i];
		int32_t material_index = static_cast<int32_t>(scene->mMeshes[mesh_index]->mMaterialIndex);

		Mesh *mesh = meshes[mesh_index];

		game::Entity entity = game::Entity(world);

		entity.addComponent<ecs::render::Transform>(toGlm(transform));
		entity.addComponent<ecs::render::Renderable>(mesh, materials[material_index]);
	}

	for (unsigned int i = 0; i < root->mNumChildren; ++i)
	{
		const aiNode *child = root->mChildren[i];
		const aiMatrix4x4 &child_transform = transform * child->mTransformation;

		importNodes(scene, child, child_transform);
	}
}
