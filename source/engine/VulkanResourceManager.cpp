#include "VulkanResourceManager.h"

#include "VulkanMesh.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"

#include <iostream>

/*
 */
VulkanMesh *VulkanResourceManager::getMesh(int id) const
{
	auto it = meshes.find(id);
	if (it != meshes.end())
		return it->second;

	return nullptr;
}

VulkanMesh *VulkanResourceManager::createCubeMesh(int id, float size)
{
	auto it = meshes.find(id);
	if (it != meshes.end())
	{
		std::cerr << "VulkanResourceManager::loadMesh(): " << id << " is already taken by another mesh" << std::endl;
		return nullptr;
	}

	VulkanMesh *mesh = new VulkanMesh(driver);
	mesh->createSkybox(size);

	meshes.insert(std::make_pair(id, mesh));
	return mesh;
}

VulkanMesh *VulkanResourceManager::loadMesh(int id, const char *path)
{
	auto it = meshes.find(id);
	if (it != meshes.end())
	{
		std::cerr << "VulkanResourceManager::loadMesh(): " << id << " is already taken by another mesh" << std::endl;
		return nullptr;
	}

	VulkanMesh *mesh = new VulkanMesh(driver);
	if (!mesh->import(path))
		return nullptr;

	meshes.insert(std::make_pair(id, mesh));
	return mesh;
}

void VulkanResourceManager::unloadMesh(int id)
{
	auto it = meshes.find(id);
	if (it == meshes.end())
		return;

	delete it->second;
	meshes.erase(it);
}

/*
 */
VulkanShader *VulkanResourceManager::getShader(int id) const
{
	auto it = shaders.find(id);
	if (it != shaders.end())
		return it->second;

	return nullptr;
}

VulkanShader *VulkanResourceManager::loadShader(int id, render::backend::ShaderType type, const char *path)
{
	auto it = shaders.find(id);
	if (it != shaders.end())
	{
		std::cerr << "VulkanResourceManager::loadShader(): " << id << " is already taken by another shader" << std::endl;
		return nullptr;
	}

	VulkanShader *shader = new VulkanShader(driver);
	if (!shader->compileFromFile(path, type))
		return nullptr;

	shaders.insert(std::make_pair(id, shader));
	return shader;
}

bool VulkanResourceManager::reloadShader(int id)
{
	auto it = shaders.find(id);
	if (it == shaders.end())
		return false;

	return it->second->reload();
}

void VulkanResourceManager::unloadShader(int id)
{
	auto it = shaders.find(id);
	if (it == shaders.end())
		return;

	delete it->second;
	shaders.erase(it);
}

/*
 */
VulkanTexture *VulkanResourceManager::getTexture(int id) const
{
	auto it = textures.find(id);
	if (it != textures.end())
		return it->second;

	return nullptr;
}

VulkanTexture *VulkanResourceManager::loadTexture(int id, const char *path)
{
	auto it = textures.find(id);
	if (it != textures.end())
	{
		std::cerr << "VulkanResourceManager::loadTexture(): " << id << " is already taken by another texture" << std::endl;
		return nullptr;
	}

	VulkanTexture *texture = new VulkanTexture(driver);
	if (!texture->import(path))
		return nullptr;

	textures.insert(std::make_pair(id, texture));
	return texture;
}

void VulkanResourceManager::unloadTexture(int id)
{
	auto it = textures.find(id);
	if (it == textures.end())
		return;

	delete it->second;
	textures.erase(it);
}
