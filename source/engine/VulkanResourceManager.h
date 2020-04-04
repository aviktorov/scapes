#pragma once

#include <unordered_map>
#include <render/backend/driver.h>

class VulkanMesh;
class VulkanShader;
class VulkanTexture;

/*
 */
class VulkanResourceManager
{
public:
	VulkanResourceManager(render::backend::Driver *driver)
		: driver(driver) {}

	VulkanMesh *getMesh(int id) const;
	VulkanMesh *createCubeMesh(int id, float size);
	VulkanMesh *loadMesh(int id, const char *path);
	void unloadMesh(int id);

	VulkanShader *getShader(int id) const;
	VulkanShader *loadShader(int id, render::backend::ShaderType type, const char *path);
	bool reloadShader(int id);
	void unloadShader(int id);

	VulkanTexture *getTexture(int id) const;
	VulkanTexture *loadTexture(int id, const char *path);
	void unloadTexture(int id);

private:
	render::backend::Driver *driver {nullptr};

	std::unordered_map<int, VulkanMesh *> meshes;
	std::unordered_map<int, VulkanShader *> shaders;
	std::unordered_map<int, VulkanTexture *> textures;
};
