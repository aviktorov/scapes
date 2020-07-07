#include "ResourceManager.h"

#include <render/Mesh.h>
#include <render/Shader.h>
#include <render/Texture.h>

#include <iostream>

/*
 */
render::Mesh *ResourceManager::getMesh(int id) const
{
	auto it = meshes.find(id);
	if (it != meshes.end())
		return it->second;

	return nullptr;
}

render::Mesh *ResourceManager::createCubeMesh(int id, float size)
{
	auto it = meshes.find(id);
	if (it != meshes.end())
	{
		std::cerr << "ResourceManager::loadMesh(): " << id << " is already taken by another mesh" << std::endl;
		return nullptr;
	}

	render::Mesh *mesh = new render::Mesh(driver);
	mesh->createSkybox(size);

	meshes.insert(std::make_pair(id, mesh));
	return mesh;
}

render::Mesh *ResourceManager::loadMesh(int id, const char *path)
{
	auto it = meshes.find(id);
	if (it != meshes.end())
	{
		std::cerr << "ResourceManager::loadMesh(): " << id << " is already taken by another mesh" << std::endl;
		return nullptr;
	}

	render::Mesh *mesh = new render::Mesh(driver);
	if (!mesh->import(path))
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
render::Shader *ResourceManager::getShader(int id) const
{
	auto it = shaders.find(id);
	if (it != shaders.end())
		return it->second;

	return nullptr;
}

render::Shader *ResourceManager::loadShader(int id, render::backend::ShaderType type, const char *path)
{
	auto it = shaders.find(id);
	if (it != shaders.end())
	{
		std::cerr << "ResourceManager::loadShader(): " << id << " is already taken by another shader" << std::endl;
		return nullptr;
	}

	render::Shader *shader = new render::Shader(driver);
	if (!shader->compileFromFile(path, type))
		return nullptr;

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
render::Texture *ResourceManager::getTexture(int id) const
{
	auto it = textures.find(id);
	if (it != textures.end())
		return it->second;

	return nullptr;
}

render::Texture *ResourceManager::loadTexture(int id, const char *path)
{
	auto it = textures.find(id);
	if (it != textures.end())
	{
		std::cerr << "ResourceManager::loadTexture(): " << id << " is already taken by another texture" << std::endl;
		return nullptr;
	}

	render::Texture *texture = new render::Texture(driver);
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
