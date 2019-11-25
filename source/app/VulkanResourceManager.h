#pragma once

#include <unordered_map>

#include "VulkanRendererContext.h"

enum class VulkanShaderKind;

class VulkanMesh;
class VulkanShader;
class VulkanTexture;

/*
 */
class VulkanResourceManager
{
public:
	VulkanResourceManager(const VulkanRendererContext &context);

	VulkanMesh *getMesh(int id) const;
	VulkanMesh *createCubeMesh(int id, float size);
	VulkanMesh *loadMesh(int id, const char *path);
	void unloadMesh(int id);

	VulkanShader *getShader(int id) const;
	VulkanShader *loadShader(int id, VulkanShaderKind kind, const char *path);
	void unloadShader(int id);

	VulkanTexture *getTexture(int id) const;
	VulkanTexture *loadTexture(int id, const char *path);
	void unloadTexture(int id);

private:
	VulkanRendererContext context;

	std::unordered_map<int, VulkanMesh *> meshes;
	std::unordered_map<int, VulkanShader *> shaders;
	std::unordered_map<int, VulkanTexture *> textures;
};
