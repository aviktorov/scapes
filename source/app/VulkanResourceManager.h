#pragma once

#include <unordered_map>

enum class VulkanShaderKind;

class VulkanContext;
class VulkanMesh;
class VulkanShader;
class VulkanTexture;

/*
 */
class VulkanResourceManager
{
public:
	VulkanResourceManager(const VulkanContext *context);

	VulkanMesh *getMesh(int id) const;
	VulkanMesh *createCubeMesh(int id, float size);
	VulkanMesh *loadMesh(int id, const char *path);
	void unloadMesh(int id);

	VulkanShader *getShader(int id) const;
	VulkanShader *loadShader(int id, const char *path);
	VulkanShader *loadShader(int id, VulkanShaderKind kind, const char *path);
	bool reloadShader(int id);
	void unloadShader(int id);

	VulkanTexture *getTexture(int id) const;
	VulkanTexture *loadTexture(int id, const char *path);
	void unloadTexture(int id);

private:
	const VulkanContext *context {nullptr};

	std::unordered_map<int, VulkanMesh *> meshes;
	std::unordered_map<int, VulkanShader *> shaders;
	std::unordered_map<int, VulkanTexture *> textures;
};
