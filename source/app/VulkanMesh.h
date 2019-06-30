#pragma once

#include <volk.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

#include <array>
#include <vector>
#include <string>

#include "VulkanRendererContext.h"

/*
 */
class VulkanMesh
{
public:
	VulkanMesh(const VulkanRendererContext &context)
		: context(context) { }

	~VulkanMesh();

	inline VkBuffer getVertexBuffer() const { return vertexBuffer; }
	inline VkBuffer getIndexBuffer() const { return indexBuffer; }
	inline uint32_t getNumIndices() const { return static_cast<uint32_t>(indices.size()); }

	static VkVertexInputBindingDescription getVertexInputBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

	bool loadFromFile(const std::string &path);

	void uploadToGPU();
	void clearGPUData();
	void clearCPUData();

private:
	void createVertexBuffer();
	void createIndexBuffer();

private:
	VulkanRendererContext context;

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 color;
		glm::vec2 uv;
	};

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	VkBuffer vertexBuffer {VK_NULL_HANDLE};
	VkDeviceMemory vertexBufferMemory {VK_NULL_HANDLE};

	VkBuffer indexBuffer {VK_NULL_HANDLE};
	VkDeviceMemory indexBufferMemory {VK_NULL_HANDLE};
};
