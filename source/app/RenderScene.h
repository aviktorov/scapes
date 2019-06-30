#pragma once

#include <volk.h>
#include <string>

#include "VulkanRendererContext.h"
#include "VulkanMesh.h"
#include "VulkanTexture.h"

/*
 */
class RenderScene
{
public:
	RenderScene(const VulkanRendererContext &context)
		: context(context), mesh(context), texture(context) { }

	void init(
		const std::string &vertexShaderFile,
		const std::string &fragmentShaderFile,
		const std::string &textureFile,
		const std::string &modelFile
	);
	void shutdown();

	inline VkShaderModule getVertexShader() const { return vertexShader; }
	inline VkShaderModule getFragmentShader() const { return fragmentShader; }
	inline const VulkanTexture &getTexture() const { return texture; }
	inline const VulkanMesh &getMesh() const { return mesh; }

private:
	VkShaderModule createShader(const std::string &path) const;

private:
	VulkanRendererContext context;
	VulkanMesh mesh;
	VulkanTexture texture;

	VkShaderModule vertexShader {VK_NULL_HANDLE};
	VkShaderModule fragmentShader {VK_NULL_HANDLE};
};
