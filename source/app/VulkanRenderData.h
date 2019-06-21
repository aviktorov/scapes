#pragma once

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

#include <array>

#include "VulkanRendererContext.h"

/*
 */
struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

/*
 */
struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 uv;

	static VkVertexInputBindingDescription getVertexInputBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

/*
 */
class RenderData
{
public:
	RenderData(const RendererContext &context)
		: context(context) { }

	void init(
		const std::string &vertexShaderFile,
		const std::string &fragmentShaderFile,
		const std::string &textureFile
	);
	void shutdown();

	inline VkShaderModule getVertexShader() const { return vertexShader; }
	inline VkShaderModule getFragmentShader() const { return fragmentShader; }
	inline VkBuffer getVertexBuffer() const { return vertexBuffer; }
	inline VkBuffer getIndexBuffer() const { return indexBuffer; }
	inline VkImageView getTextureImageView() const { return textureImageView; }
	inline VkSampler getTextureImageSampler() const { return textureImageSampler; }
	uint32_t getNumIndices() const;

private:
	VkShaderModule createShader(const std::string &path) const;
	void createVertexBuffer();
	void createIndexBuffer();
	void createImage(const std::string &path);

private:
	RendererContext context;

	VkShaderModule vertexShader {VK_NULL_HANDLE};
	VkShaderModule fragmentShader {VK_NULL_HANDLE};

	VkBuffer vertexBuffer {VK_NULL_HANDLE};
	VkDeviceMemory vertexBufferMemory {VK_NULL_HANDLE};

	VkBuffer indexBuffer {VK_NULL_HANDLE};
	VkDeviceMemory indexBufferMemory {VK_NULL_HANDLE};

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

	VkImageView textureImageView;
	VkSampler textureImageSampler;
};
