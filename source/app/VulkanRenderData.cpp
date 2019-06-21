#include "VulkanRenderData.h"
#include "VulkanUtils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>
#include <iostream>
#include <vector>

const std::vector<Vertex> vertices = {
	{ {-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
	{ {0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
	{ {0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
	{ {-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f} },

	{ {-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
	{ {0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
	{ {0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
	{ {-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f} },
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
};

/*
 */
static std::vector<char> readFile(const std::string &filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("Can't open file");

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

/*
 */
VkVertexInputBindingDescription Vertex::getVertexInputBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription;
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attributes = {};

	attributes[0].binding = 0;
	attributes[0].location = 0;
	attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributes[0].offset = offsetof(Vertex, position);

	attributes[1].binding = 0;
	attributes[1].location = 1;
	attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributes[1].offset = offsetof(Vertex, color);

	attributes[2].binding = 0;
	attributes[2].location = 2;
	attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributes[2].offset = offsetof(Vertex, uv);

	return attributes;
}

/*
 */
void RenderData::init(const std::string &vertexShaderFile, const std::string &fragmentShaderFile, const std::string &textureFile)
{
	vertexShader = createShader(vertexShaderFile);
	fragmentShader = createShader(fragmentShaderFile);

	createImage(textureFile);
	createVertexBuffer();
	createIndexBuffer();
}

void RenderData::shutdown()
{
	vkDestroySampler(context.device, textureImageSampler, nullptr);
	textureImageSampler = nullptr;

	vkDestroyImageView(context.device, textureImageView, nullptr);
	textureImageView = nullptr;

	vkDestroyImage(context.device, textureImage, nullptr);
	textureImage = nullptr;

	vkFreeMemory(context.device, textureImageMemory, nullptr);
	textureImageMemory = nullptr;

	vkDestroyBuffer(context.device, vertexBuffer, nullptr);
	vertexBuffer = VK_NULL_HANDLE;

	vkFreeMemory(context.device, vertexBufferMemory, nullptr);
	vertexBufferMemory = VK_NULL_HANDLE;

	vkDestroyBuffer(context.device, indexBuffer, nullptr);
	indexBuffer = VK_NULL_HANDLE;

	vkFreeMemory(context.device, indexBufferMemory, nullptr);
	indexBufferMemory = VK_NULL_HANDLE;

	vkDestroyShaderModule(context.device, vertexShader, nullptr);
	vertexShader = VK_NULL_HANDLE;

	vkDestroyShaderModule(context.device, fragmentShader, nullptr);
	fragmentShader = VK_NULL_HANDLE;
}

/*
 */
uint32_t RenderData::getNumIndices() const
{
	return static_cast<uint32_t>(indices.size());
}

/*
 */
VkShaderModule RenderData::createShader(const std::string &path) const
{
	std::vector<char> code = readFile(path);

	VkShaderModuleCreateInfo shaderInfo = {};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.codeSize = code.size();
	shaderInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

	VkShaderModule shader;
	if (vkCreateShaderModule(context.device, &shaderInfo, nullptr, &shader) != VK_SUCCESS)
		throw std::runtime_error("Can't create shader module");

	return shader;
}

/*
 */
void RenderData::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

	VulkanUtils::createBuffer(
		context,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer,
		vertexBufferMemory
	);

	// Create staging buffer
	VulkanUtils::createBuffer(
		context,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	// Fill staging buffer
	void *data = nullptr;
	vkMapMemory(context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(context.device, stagingBufferMemory);

	// Transfer to GPU local memory
	VulkanUtils::copyBuffer(context, stagingBuffer, vertexBuffer, bufferSize);

	// Destroy staging buffer
	vkDestroyBuffer(context.device, stagingBuffer, nullptr);
	vkFreeMemory(context.device, stagingBufferMemory, nullptr);
}

void RenderData::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(uint16_t) * indices.size();

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

	VulkanUtils::createBuffer(
		context,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer,
		indexBufferMemory
	);

	// Create staging buffer
	VulkanUtils::createBuffer(
		context,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	// Fill staging buffer
	void *data = nullptr;
	vkMapMemory(context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(context.device, stagingBufferMemory);

	// Transfer to GPU local memory
	VulkanUtils::copyBuffer(context, stagingBuffer, indexBuffer, bufferSize);

	// Destroy staging buffer
	vkDestroyBuffer(context.device, stagingBuffer, nullptr);
	vkFreeMemory(context.device, stagingBufferMemory, nullptr);
}

void RenderData::createImage(const std::string &path)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	// Pixel data will have alpha channel even if the original image doesn't
	VkDeviceSize imageSize = texWidth * texHeight * 4; 

	if (!pixels)
		throw std::runtime_error("failed to load texture image!");

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

	VulkanUtils::createBuffer(
		context,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	// Fill staging buffer
	void *data = nullptr;
	vkMapMemory(context.device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(context.device, stagingBufferMemory);

	stbi_image_free(pixels);
	pixels = nullptr;

	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	VulkanUtils::createImage2D(
		context,
		texWidth,
		texHeight,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage,
		textureImageMemory
	);

	// Prepare the image for transfer
	VulkanUtils::transitionImageLayout(
		context,
		textureImage,
		format,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	// Copy to the image memory on GPU
	VulkanUtils::copyBufferToImage(
		context,
		stagingBuffer,
		textureImage,
		texWidth,
		texHeight
	);

	// Prepare the image for shader access
	VulkanUtils::transitionImageLayout(
		context,
		textureImage,
		format,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	// Destroy staging buffer
	vkDestroyBuffer(context.device, stagingBuffer, nullptr);
	vkFreeMemory(context.device, stagingBufferMemory, nullptr);

	// Create image view & sampler
	textureImageView = VulkanUtils::createImage2DView(context, textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
	textureImageSampler = VulkanUtils::createSampler(context);
}
