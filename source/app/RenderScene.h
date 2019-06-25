#pragma once

#include <vulkan/vulkan.h>

#include <string>

#include "VulkanRendererContext.h"
#include "VulkanMesh.h"

/*
 */
class RenderScene
{
public:
	RenderScene(const VulkanRendererContext &context)
		: context(context), mesh(context) { }

	void init(
		const std::string &vertexShaderFile,
		const std::string &fragmentShaderFile,
		const std::string &textureFile,
		const std::string &modelFile
	);
	void shutdown();

	inline VkShaderModule getVertexShader() const { return vertexShader; }
	inline VkShaderModule getFragmentShader() const { return fragmentShader; }
	inline VkImageView getTextureImageView() const { return textureImageView; }
	inline VkSampler getTextureImageSampler() const { return textureImageSampler; }
	inline const VulkanMesh &getMesh() const { return mesh; }

private:
	VkShaderModule createShader(const std::string &path) const;
	void createImage(const std::string &path);

private:
	VulkanRendererContext context;
	VulkanMesh mesh;

	VkShaderModule vertexShader {VK_NULL_HANDLE};
	VkShaderModule fragmentShader {VK_NULL_HANDLE};

	VkImage textureImage {VK_NULL_HANDLE};
	VkDeviceMemory textureImageMemory {VK_NULL_HANDLE};
	VkImageView textureImageView {VK_NULL_HANDLE};
	VkSampler textureImageSampler {VK_NULL_HANDLE};
};
