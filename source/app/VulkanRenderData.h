#pragma once

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <string>

#include "VulkanRendererContext.h"
#include "VulkanMesh.h"

/*
 */
class RenderData
{
public:
	RenderData(const RendererContext &context)
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
	RendererContext context;
	VulkanMesh mesh;

	VkShaderModule vertexShader {VK_NULL_HANDLE};
	VkShaderModule fragmentShader {VK_NULL_HANDLE};

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

	VkImageView textureImageView;
	VkSampler textureImageSampler;
};
