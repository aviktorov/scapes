#include "ResourceManager.h"

#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"

#include <iostream>

/*
 */
ResourceManager::~ResourceManager()
{
	clear();
}

/*
 */
void ResourceManager::clear()
{
	for (auto it : meshes)
		delete it.second;

	for (auto it : shaders)
		delete it.second;

	for (auto it : textures)
		delete it.second;

	meshes.clear();
	shaders.clear();
	textures.clear();
}

/*
 */
Mesh *ResourceManager::getMesh(int id) const
{
	auto it = meshes.find(id);
	if (it != meshes.end())
		return it->second;

	return nullptr;
}

Mesh *ResourceManager::createCubeMesh(int id, float size)
{
	auto it = meshes.find(id);
	if (it != meshes.end())
	{
		std::cerr << "ResourceManager::loadMesh(): " << id << " is already taken by another mesh" << std::endl;
		return nullptr;
	}

	Mesh *mesh = new Mesh(driver);
	mesh->createSkybox(size);

	meshes.insert(std::make_pair(id, mesh));
	return mesh;
}

Mesh *ResourceManager::loadMesh(int id, const char *path)
{
	auto it = meshes.find(id);
	if (it != meshes.end())
	{
		std::cerr << "ResourceManager::loadMesh(): " << id << " is already taken by another mesh" << std::endl;
		return nullptr;
	}

	Mesh *mesh = new Mesh(driver);
	if (!mesh->importAssimp(path))
		return nullptr;

	meshes.insert(std::make_pair(id, mesh));
	return mesh;
}

void ResourceManager::unloadMesh(int id)
{
	auto it = meshes.find(id);
	if (it == meshes.end())
		return;

	delete it->second;
	meshes.erase(it);
}

/*
 */
Shader *ResourceManager::getShader(int id) const
{
	auto it = shaders.find(id);
	if (it != shaders.end())
		return it->second;

	return nullptr;
}

Shader *ResourceManager::loadShader(int id, render::backend::ShaderType type, const char *path)
{
	auto it = shaders.find(id);
	if (it != shaders.end())
	{
		std::cerr << "ResourceManager::loadShader(): " << id << " is already taken by another shader" << std::endl;
		return nullptr;
	}

	Shader *shader = new Shader(driver, compiler);
	if (!shader->compileFromFile(type, path))
	{
		// TODO: log warning
	}

	shaders.insert(std::make_pair(id, shader));
	return shader;
}

bool ResourceManager::reloadShader(int id)
{
	auto it = shaders.find(id);
	if (it == shaders.end())
		return false;

	return it->second->reload();
}

void ResourceManager::unloadShader(int id)
{
	auto it = shaders.find(id);
	if (it == shaders.end())
		return;

	delete it->second;
	shaders.erase(it);
}

/*
 */
Texture *ResourceManager::getTexture(int id) const
{
	auto it = textures.find(id);
	if (it != textures.end())
		return it->second;

	return nullptr;
}

Texture *ResourceManager::loadTexture(int id, const char *path)
{
	auto it = textures.find(id);
	if (it != textures.end())
	{
		std::cerr << "ResourceManager::loadTexture(): " << id << " is already taken by another texture" << std::endl;
		return nullptr;
	}

	Texture *texture = new Texture(driver);
	if (!texture->import(path))
		return nullptr;

	textures.insert(std::make_pair(id, texture));
	return texture;
}

void ResourceManager::unloadTexture(int id)
{
	auto it = textures.find(id);
	if (it == textures.end())
		return;

	delete it->second;
	textures.erase(it);
}
