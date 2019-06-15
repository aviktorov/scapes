#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <array>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <vector>

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
	glm::vec2 position;
	glm::vec3 color;
	glm::vec2 uv;

	static VkVertexInputBindingDescription getVertexInputBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription;
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributes = {};

		attributes[0].binding = 0;
		attributes[0].location = 0;
		attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
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
};

const std::vector<Vertex> vertices = {
	{ {-0.5f, -0.5f},	{1.0f, 0.0f, 0.0f},	{1.0f, 0.0f} },
	{ {0.5f, -0.5f},	{0.0f, 1.0f, 0.0f},	{0.0f, 0.0f} },
	{ {0.5f, 0.5f},		{0.0f, 0.0f, 1.0f},	{0.0f, 1.0f} },
	{ {-0.5f, 0.5f},	{1.0f, 1.0f, 1.0f},	{1.0f, 1.0f} },
};

const std::vector<uint16_t> indices =
{
	0, 1, 2,
	2, 3, 0,
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
struct QueueFamilyIndices
{
	std::pair<bool, uint32_t> graphicsFamily { std::make_pair(false, 0) };
	std::pair<bool, uint32_t> presentFamily { std::make_pair(false, 0) };

	bool isComplete() {
		return graphicsFamily.first && presentFamily.first;
	}
};

/*
 */
struct RendererContext
{
	VkDevice device {VK_NULL_HANDLE};
	VkPhysicalDevice physicalDevice {VK_NULL_HANDLE};
	VkCommandPool commandPool {VK_NULL_HANDLE};
	VkDescriptorPool descriptorPool;
	VkFormat format;
	VkExtent2D extent;
	std::vector<VkImageView> imageViews;
	VkQueue graphicsQueue {VK_NULL_HANDLE};
	VkQueue presentQueue {VK_NULL_HANDLE};
};

/*
 */
class VulkanUtils
{
public:
	static uint32_t findMemoryType(
		const RendererContext &context,
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties
	);

	static void createBuffer(
		const RendererContext &context,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags memoryProperties,
		VkBuffer &buffer,
		VkDeviceMemory &memory
	);

	static void createImage2D(
		const RendererContext &context,
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags memoryProperties,
		VkImage &image,
		VkDeviceMemory &memory
	);

	static VkImageView createImage2DView(
		const RendererContext &context,
		VkImage image,
		VkFormat format
	);

	static VkSampler createSampler(
		const RendererContext &context
	);

	static void copyBuffer(
		const RendererContext &context,
		VkBuffer src,
		VkBuffer dst,
		VkDeviceSize size
	);

	static void copyBufferToImage(
		const RendererContext &context,
		VkBuffer src,
		VkImage dst,
		uint32_t width,
		uint32_t height
	);

	static void transitionImageLayout(
		const RendererContext &context,
		VkImage image,
		VkImageLayout oldLayout,
		VkImageLayout newLayout
	);

private:

	static VkCommandBuffer beginSingleTimeCommands(const RendererContext &context);
	static void endSingleTimeCommands(const RendererContext &context, VkCommandBuffer commandBuffer);
};

/*
 */
uint32_t VulkanUtils::findMemoryType(
	const RendererContext &context,
	uint32_t typeFilter,
	VkMemoryPropertyFlags properties
)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(context.physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		uint32_t memoryTypeProperties = memProperties.memoryTypes[i].propertyFlags;
		if ((typeFilter & (1 << i)) && (memoryTypeProperties & properties) == properties)
			return i;
	}

	throw std::runtime_error("Can't find suitable memory type");
}

void VulkanUtils::createBuffer(
	const RendererContext &context,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags memoryProperties,
	VkBuffer &buffer,
	VkDeviceMemory &memory
)
{
	// Create buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(context.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("Can't create buffer");

	// Allocate memory for the buffer
	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(context.device, buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(context, memoryRequirements.memoryTypeBits, memoryProperties);

	if (vkAllocateMemory(context.device, &memoryAllocateInfo, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error("Can't allocate buffer memory");

	if (vkBindBufferMemory(context.device, buffer, memory, 0) != VK_SUCCESS)
		throw std::runtime_error("Can't bind buffer memory");
}

void VulkanUtils::createImage2D(
	const RendererContext &context,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags memoryProperties,
	VkImage &image,
	VkDeviceMemory &memory
)
{
	// Create buffer
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.usage = usage;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional

	if (vkCreateImage(context.device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("Can't create image");

	// Allocate memory for the buffer
	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(context.device, image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(context, memoryRequirements.memoryTypeBits, memoryProperties);

	if (vkAllocateMemory(context.device, &memoryAllocateInfo, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error("Can't allocate image memory");

	if (vkBindImageMemory(context.device, image, memory, 0) != VK_SUCCESS)
		throw std::runtime_error("Can't bind image memory");
}

VkImageView VulkanUtils::createImage2DView(const RendererContext &context, VkImage image, VkFormat format)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(context.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("Can't create image view!");

	return imageView;
}

VkSampler VulkanUtils::createSampler(const RendererContext &context)
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	VkSampler sampler = VK_NULL_HANDLE;
	if (vkCreateSampler(context.device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
		throw std::runtime_error("Can't create texture sampler");

	return sampler;
}

void VulkanUtils::copyBuffer(
	const RendererContext &context,
	VkBuffer src,
	VkBuffer dst,
	VkDeviceSize size
)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(context);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

	endSingleTimeCommands(context, commandBuffer);
}


void VulkanUtils::copyBufferToImage(
	const RendererContext &context,
	VkBuffer src,
	VkImage dst,
	uint32_t width,
	uint32_t height
)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(context);

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(commandBuffer, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(context, commandBuffer);
}

void VulkanUtils::transitionImageLayout(
	const RendererContext &context,
	VkImage image,
	VkImageLayout oldLayout,
	VkImageLayout newLayout
)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(context);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
		throw std::runtime_error("Unsupported layout transition");

	vkCmdPipelineBarrier(
		commandBuffer,
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(context, commandBuffer);
}

/*
 */
VkCommandBuffer VulkanUtils::beginSingleTimeCommands(const RendererContext &context)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = context.commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(context.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanUtils::endSingleTimeCommands(const RendererContext &context, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(context.graphicsQueue);

	vkFreeCommandBuffers(context.device, context.commandPool, 1, &commandBuffer);
}

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

	inline VkShaderModule GetVertexShader() const { return vertexShader; }
	inline VkShaderModule GetFragmentShader() const { return fragmentShader; }
	inline VkBuffer GetVertexBuffer() const { return vertexBuffer; }
	inline VkBuffer GetIndexBuffer() const { return indexBuffer; }
	inline VkImageView GetTextureImageView() const { return textureImageView; }
	inline VkSampler GetTextureImageSampler() const { return textureImageSampler; }
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
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	// Destroy staging buffer
	vkDestroyBuffer(context.device, stagingBuffer, nullptr);
	vkFreeMemory(context.device, stagingBufferMemory, nullptr);

	// Create image view & sampler
	textureImageView = VulkanUtils::createImage2DView(context, textureImage, format);
	textureImageSampler = VulkanUtils::createSampler(context);
}

/*
 */
class Renderer
{
public:
	Renderer(const RendererContext &context)
		: context(context), data(context) { }

	void init(
		const std::string &vertexShaderFile,
		const std::string &fragmentShaderFile,
		const std::string &textureFile
	);
	VkCommandBuffer render(uint32_t imageIndex);
	void shutdown();

private:
	VkShaderModule createShader(const std::string &path) const;

private:
	RenderData data;
	RendererContext context;

	VkRenderPass renderPass {VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptorSetLayout {VK_NULL_HANDLE};
	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
	VkPipeline pipeline {VK_NULL_HANDLE};

	std::vector<VkFramebuffer> frameBuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	std::vector<VkDescriptorSet> descriptorSets;
};

/*
 */
void Renderer::init(const std::string &vertexShaderFile, const std::string &fragmentShaderFile, const std::string &textureFile)
{
	data.init(vertexShaderFile, fragmentShaderFile, textureFile);

	// Create uniform buffers
	VkDeviceSize uboSize = sizeof(UniformBufferObject);

	uint32_t imageCount = static_cast<uint32_t>(context.imageViews.size());
	uniformBuffers.resize(imageCount);
	uniformBuffersMemory.resize(imageCount);

	for (uint32_t i = 0; i < imageCount; i++)
	{
		VulkanUtils::createBuffer(
			context,
			uboSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffers[i],
			uniformBuffersMemory[i]
		);
	}

	// Create shader stages
	VkPipelineShaderStageCreateInfo vertexShaderStageInfo = {};
	vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageInfo.module = data.GetVertexShader();
	vertexShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {};
	fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageInfo.module = data.GetFragmentShader();
	fragmentShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

	// Create vertex input
	VkVertexInputBindingDescription bindingDescription = Vertex::getVertexInputBindingDescription();
	auto attributes = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

	// Create input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	// Create viewport state
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(context.extent.width);
	viewport.height = static_cast<float>(context.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = context.extent;

	VkPipelineViewportStateCreateInfo viewportStateInfo = {};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.pViewports = &viewport;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.pScissors = &scissor;

	// Create rasterizer state
	VkPipelineRasterizationStateCreateInfo rasterizerInfo = {};
	rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerInfo.depthClampEnable = VK_FALSE; // VK_TRUE value require enabling GPU feature
	rasterizerInfo.rasterizerDiscardEnable = VK_FALSE; 
	rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL; // other modes require enabling GPU feature
	rasterizerInfo.lineWidth = 1.0f; // other values require enabling GPU feature
	rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerInfo.depthBiasEnable = VK_FALSE;
	rasterizerInfo.depthBiasConstantFactor = 0.0f;
	rasterizerInfo.depthBiasClamp = 0.0f;
	rasterizerInfo.depthBiasSlopeFactor = 0.0f;

	// Create multisampling state (enabling it requires enabling GPU feature)
	VkPipelineMultisampleStateCreateInfo multisamplingInfo = {};
	multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingInfo.sampleShadingEnable = VK_FALSE;
	multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingInfo.minSampleShading = 1.0f; // Optional
	multisamplingInfo.pSampleMask = nullptr; // Optional
	multisamplingInfo.alphaToCoverageEnable = VK_FALSE; // Optional
	multisamplingInfo.alphaToOneEnable = VK_FALSE; // Optional

	// Create depth stencil state (won't be used now)
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};

	// Create color blend state
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlendingInfo = {};
	colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingInfo.logicOpEnable = VK_FALSE;
	colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingInfo.attachmentCount = 1;
	colorBlendingInfo.pAttachments = &colorBlendAttachment;
	colorBlendingInfo.blendConstants[0] = 0.0f;
	colorBlendingInfo.blendConstants[1] = 0.0f;
	colorBlendingInfo.blendConstants[2] = 0.0f;
	colorBlendingInfo.blendConstants[3] = 0.0f;

	// Create pipeline dynamic state (won't be used at the moment)
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = 2;
	dynamicStateInfo.pDynamicStates = dynamicStates;

	// Create descriptor set layout
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Can't create descriptor set layout");

	// Create descriptor sets
	std::vector<VkDescriptorSetLayout> layouts(imageCount, descriptorSetLayout);

	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.descriptorPool = context.descriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = imageCount;
	descriptorSetAllocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(imageCount);
	if (vkAllocateDescriptorSets(context.device, &descriptorSetAllocInfo, descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Can't allocate descriptor sets");

	for (size_t i = 0; i < imageCount; i++)
	{
		VkDescriptorBufferInfo descriptorBufferInfo = {};
		descriptorBufferInfo.buffer = uniformBuffers[i];
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo descriptorImageInfo = {};
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImageInfo.imageView = data.GetTextureImageView();
		descriptorImageInfo.sampler = data.GetTextureImageSampler();

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &descriptorBufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &descriptorImageInfo;

		vkUpdateDescriptorSets(
			context.device,
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0,
			nullptr
		);
	}

	// Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Can't create pipeline layout");

	// Create render pass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = context.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		throw std::runtime_error("Can't create render pass");

	// Create graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &rasterizerInfo;
	pipelineInfo.pMultisampleState = &multisamplingInfo;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlendingInfo;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
		throw std::runtime_error("Can't create graphics pipeline");

	// Create framebuffers
	frameBuffers.resize(imageCount);
	for (size_t i = 0; i < imageCount; i++) {
		VkImageView attachments[] = {
			context.imageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = context.extent.width;
		framebufferInfo.height = context.extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(context.device, &framebufferInfo, nullptr, &frameBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Can't create framebuffer");
	}

	// Create command buffers
	commandBuffers.resize(imageCount);

	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = context.commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(context.device, &allocateInfo, commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("Can't create command buffers");

	// Record command buffers
	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Can't begin recording command buffer");

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = frameBuffers[i];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = context.extent;

		VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

		VkBuffer vertexBuffers[] = { data.GetVertexBuffer() };
		VkBuffer indexBuffer = data.GetIndexBuffer();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Can't record command buffer");
	}
}

void Renderer::shutdown()
{
	data.shutdown();

	for (auto uniformBuffer : uniformBuffers)
		vkDestroyBuffer(context.device, uniformBuffer, nullptr);
	
	uniformBuffers.clear();

	for (auto uniformBufferMemory : uniformBuffersMemory)
		vkFreeMemory(context.device, uniformBufferMemory, nullptr);

	uniformBuffersMemory.clear();

	for (auto framebuffer : frameBuffers)
		vkDestroyFramebuffer(context.device, framebuffer, nullptr);

	frameBuffers.clear();

	vkDestroyPipeline(context.device, pipeline, nullptr);
	pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(context.device, pipelineLayout, nullptr);
	pipelineLayout = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(context.device, descriptorSetLayout, nullptr);
	descriptorSetLayout = nullptr;

	vkDestroyRenderPass(context.device, renderPass, nullptr);
	renderPass = VK_NULL_HANDLE;
}

/*
 */
VkCommandBuffer Renderer::render(uint32_t imageIndex)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();

	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	VkBuffer uniformBuffer = uniformBuffers[imageIndex];
	VkDeviceMemory uniformBufferMemory = uniformBuffersMemory[imageIndex];

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), context.extent.width / (float) context.extent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	void *data;
	vkMapMemory(context.device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(context.device, uniformBufferMemory);

	return commandBuffers[imageIndex];
}

/*
 */
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

/*
 */
struct SwapChainSettings
{
	VkSurfaceFormatKHR format;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
};

/*
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

/*
 */
class Application
{
public:
	void run();

private:
	void initWindow();
	void shutdownWindow();

	bool checkRequiredValidationLayers(std::vector<const char *> &layers) const;
	bool checkRequiredExtensions(std::vector<const char *> &extensions) const;
	bool checkRequiredPhysicalDeviceExtensions(VkPhysicalDevice device, std::vector<const char *> &extensions) const;
	bool checkPhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface) const;

	SwapChainSupportDetails fetchSwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface) const;
	QueueFamilyIndices fetchQueueFamilyIndices(VkPhysicalDevice device) const;

	SwapChainSettings selectOptimalSwapChainSettings(const SwapChainSupportDetails &details) const;

	void initVulkanExtensions();

	void initVulkan();
	void shutdownVulkan();

	void initRenderer();
	void shutdownRenderer();

	void render();
	void mainloop();

private:
	GLFWwindow *window {nullptr};
	Renderer *renderer {nullptr};

	VkInstance instance {VK_NULL_HANDLE};
	VkDebugUtilsMessengerEXT debugMessenger {VK_NULL_HANDLE};
	VkPhysicalDevice physicalDevice {VK_NULL_HANDLE};
	VkDevice device {VK_NULL_HANDLE};
	VkQueue graphicsQueue {VK_NULL_HANDLE};
	VkQueue presentQueue {VK_NULL_HANDLE};
	VkSurfaceKHR surface {VK_NULL_HANDLE};

	VkSwapchainKHR swapChain {VK_NULL_HANDLE};
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;

	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	VkDescriptorPool descriptorPool;
	VkCommandPool commandPool {VK_NULL_HANDLE};

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	size_t currentFrame {0};

	enum
	{
		WIDTH = 640,
		HEIGHT = 480,
	};

	enum
	{
		MAX_FRAMES_IN_FLIGHT = 2,
	};

	static std::vector<const char*> requiredPhysicalDeviceExtensions;
	static std::vector<const char *> requiredValidationLayers;

	static PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugMessenger;
	static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugMessenger;
};

/*
 */
std::vector<const char*> Application::requiredPhysicalDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

std::vector<const char *> Application::requiredValidationLayers = {
	"VK_LAYER_KHRONOS_validation",
};

PFN_vkCreateDebugUtilsMessengerEXT Application::vkCreateDebugMessenger {VK_NULL_HANDLE};
PFN_vkDestroyDebugUtilsMessengerEXT Application::vkDestroyDebugMessenger {VK_NULL_HANDLE};

/*
 */
void Application::run()
{
	initWindow();
	initVulkan();
	initRenderer();
	mainloop();
	shutdownRenderer();
	shutdownVulkan();
	shutdownWindow();
}

/*
 */
void Application::render()
{
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(device, 1, &inFlightFences[currentFrame]);
	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	VkCommandBuffer commandBuffer = renderer->render(imageIndex);

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("Can't submit command buffer");

	VkSwapchainKHR swapChains[] = {swapChain};
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	vkQueuePresentKHR(presentQueue, &presentInfo);
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/*
 */
void Application::mainloop()
{
	if (!window)
		return;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		render();
	}

	vkDeviceWaitIdle(device);
}

/*
 */
void Application::initWindow()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void Application::shutdownWindow()
{
	glfwDestroyWindow(window);
	window = nullptr;
}

/*
 */
void Application::initRenderer()
{
	RendererContext context = {};
	context.device = device;
	context.physicalDevice = physicalDevice;
	context.commandPool = commandPool;
	context.descriptorPool = descriptorPool;
	context.format = swapChainImageFormat;
	context.extent = swapChainExtent;
	context.imageViews = swapChainImageViews;
	context.graphicsQueue = graphicsQueue;
	context.presentQueue = presentQueue;

	renderer = new Renderer(context);
	renderer->init(
		"D:/Development/Projects/pbr-sandbox/shaders/vert.spv",
		"D:/Development/Projects/pbr-sandbox/shaders/frag.spv",
		"D:/Development/Projects/pbr-sandbox/textures/texture.jpg"
	);
}

void Application::shutdownRenderer()
{
	renderer->shutdown();

	delete renderer;
	renderer = nullptr;
}

/*
 */
bool Application::checkRequiredValidationLayers(std::vector<const char *> &layers) const
{
	uint32_t vulkanLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&vulkanLayerCount, nullptr);
	std::vector<VkLayerProperties> vulkanLayers(vulkanLayerCount);
	vkEnumerateInstanceLayerProperties(&vulkanLayerCount, vulkanLayers.data());

	layers.clear();
	for (const auto &requiredLayer : requiredValidationLayers)
	{
		bool supported = false;
		for (const auto &vulkanLayer : vulkanLayers)
		{
			if (strcmp(requiredLayer, vulkanLayer.layerName) == 0)
			{
				supported = true;
				break;
			}
		}

		if (!supported)
		{
			std::cerr << requiredLayer << " is not supported on this device" << std::endl;
			return false;
		}
		
		std::cout << "Have " << requiredLayer << std::endl;
		layers.push_back(requiredLayer);
	}

	return true;
}

bool Application::checkRequiredExtensions(std::vector<const char *> &extensions) const
{
	uint32_t vulkanExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &vulkanExtensionCount, nullptr);
	std::vector<VkExtensionProperties> vulkanExtensions(vulkanExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &vulkanExtensionCount, vulkanExtensions.data());

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char *> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	extensions.clear();
	for (const auto &requiredExtension : requiredExtensions)
	{
		bool supported = false;
		for (const auto &vulkanExtension : vulkanExtensions)
		{
			if (strcmp(requiredExtension, vulkanExtension.extensionName) == 0)
			{
				supported = true;
				break;
			}
		}

		if (!supported)
		{
			std::cerr << requiredExtension << " is not supported on this device" << std::endl;
			return false;
		}
		
		std::cout << "Have " << requiredExtension << std::endl;
		extensions.push_back(requiredExtension);
	}

	return true;
}

bool Application::checkRequiredPhysicalDeviceExtensions(VkPhysicalDevice device, std::vector<const char *> &extensions) const
{
	uint32_t deviceExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);
	std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, deviceExtensions.data());

	extensions.clear();
	for (const char *requiredExtension : requiredPhysicalDeviceExtensions)
	{
		bool supported = false;
		for (const auto &deviceExtension : deviceExtensions)
		{
			if (strcmp(requiredExtension, deviceExtension.extensionName) == 0)
			{
				supported = true;
				break;
			}
		}

		if (!supported)
		{
			std::cerr << requiredExtension << " is not supported on this physical device" << std::endl;
			return false;
		}

		std::cout << "Have " << requiredExtension << std::endl;
		extensions.push_back(requiredExtension);
	}

	return true;
}

bool Application::checkPhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
	QueueFamilyIndices indices = fetchQueueFamilyIndices(device);
	if (!indices.isComplete())
		return false;

	std::vector<const char *> deviceExtensions;
	if (!checkRequiredPhysicalDeviceExtensions(device, deviceExtensions))
		return false;

	SwapChainSupportDetails details = fetchSwapChainSupportDetails(device, surface);
	if (details.formats.empty() || details.presentModes.empty())
		return false;

	// TODO: These checks are for testing purposes only, remove later
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	if (!deviceFeatures.geometryShader)
		return false;

	if (!deviceFeatures.samplerAnisotropy)
		return false;

	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

/*
 */
QueueFamilyIndices Application::fetchQueueFamilyIndices(VkPhysicalDevice device) const
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	QueueFamilyIndices indices = {};

	for (const auto &queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = std::make_pair(true, i);

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport)
			indices.presentFamily = std::make_pair(true, i);

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

SwapChainSupportDetails Application::fetchSwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount > 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount > 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

/*
 */
SwapChainSettings Application::selectOptimalSwapChainSettings(const SwapChainSupportDetails &details) const
{
	assert(!details.formats.empty());
	assert(!details.presentModes.empty());

	SwapChainSettings settings;

	// Select the best format if the surface has no preferred format
	if (details.formats.size() == 1 && details.formats[0].format == VK_FORMAT_UNDEFINED)
	{
		settings.format = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	// Otherwise, select one of the available formats
	else
	{
		settings.format = details.formats[0];
		for (const auto &format : details.formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				settings.format = format;
				break;
			}
		}
	}

	// Select the best present mode
	settings.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto &presentMode : details.presentModes)
	{
		// Some drivers currently don't properly support FIFO present mode,
		// so we should prefer IMMEDIATE mode if MAILBOX mode is not available
		if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			settings.presentMode = presentMode;

		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			settings.presentMode = presentMode;
			break;
		}
	}

	// Select current swap extent if window manager doesn't allow to set custom extent
	if (details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		settings.extent = details.capabilities.currentExtent;
	}
	// Otherwise, manually set extent to match the min/max extent bounds
	else
	{
		const VkSurfaceCapabilitiesKHR &capabilities = details.capabilities;

		settings.extent = { WIDTH, HEIGHT };
		settings.extent.width = std::max(
			capabilities.minImageExtent.width,
			std::min(settings.extent.width, capabilities.maxImageExtent.width)
		);
		settings.extent.height = std::max(
			capabilities.minImageExtent.height,
			std::min(settings.extent.height, capabilities.maxImageExtent.height)
		);
	}

	return settings;
}

/*
 */
void Application::initVulkan()
{
	// Check required extensions & layers
	std::vector<const char *> extensions;
	if (!checkRequiredExtensions(extensions))
		throw std::runtime_error("This device doesn't have required Vulkan extensions");

	std::vector<const char *> layers;
	if (!checkRequiredValidationLayers(layers))
		throw std::runtime_error("This device doesn't have required Vulkan validation layers");

	// Fill instance structures
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "PBR Sandbox";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
	debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugMessengerInfo.pfnUserCallback = debugCallback;
	debugMessengerInfo.pUserData = nullptr;

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceInfo.ppEnabledExtensionNames = extensions.data();
	instanceInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
	instanceInfo.ppEnabledLayerNames = layers.data();
	instanceInfo.pNext = &debugMessengerInfo;

	// Create Vulkan instance
	VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create Vulkan instance");

	initVulkanExtensions();

	// Create Vulkan debug messenger
	result = vkCreateDebugMessenger(instance, &debugMessengerInfo, nullptr, &debugMessenger);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Can't create Vulkan debug messenger");

	// Create Vulkan win32 surface
	VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
	surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceInfo.hwnd = glfwGetWin32Window(window);
	surfaceInfo.hinstance = GetModuleHandle(nullptr);

	result = vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, &surface);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Can't create Vulkan win32 surface KHR");

	// Enumerate physical devices
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		throw std::runtime_error("Failed to find GPUs with Vulkan support");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (checkPhysicalDevice(device, surface))
		{
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to find a suitable GPU");

	// TODO: scores

	// Create logical device
	QueueFamilyIndices indices = fetchQueueFamilyIndices(physicalDevice);

	const float queuePriority = 1.0f;

	std::vector<VkDeviceQueueCreateInfo> queuesInfo;
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.second, indices.presentFamily.second};

	for (uint32_t queueFamilyIndex : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.queueFamilyIndex = queueFamilyIndex;
		info.queueCount = 1;
		info.pQueuePriorities = &queuePriority;
		queuesInfo.push_back(info);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queuesInfo.size());
	deviceCreateInfo.pQueueCreateInfos = queuesInfo.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredPhysicalDeviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = requiredPhysicalDeviceExtensions.data();

	// next two parameters are ignored, but it's still good to pass layers for backward compatibility
	deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size()); 
	deviceCreateInfo.ppEnabledLayerNames = layers.data();

	result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Can't create logical device");

	// Get logical device queues
	vkGetDeviceQueue(device, indices.graphicsFamily.second, 0, &graphicsQueue);
	if (graphicsQueue == VK_NULL_HANDLE)
		throw std::runtime_error("Can't get graphics queue from logical device");

	vkGetDeviceQueue(device, indices.presentFamily.second, 0, &presentQueue);
	if (presentQueue == VK_NULL_HANDLE)
		throw std::runtime_error("Can't get present queue from logical device");

	// Create swap chain
	SwapChainSupportDetails details = fetchSwapChainSupportDetails(physicalDevice, surface);
	SwapChainSettings settings = selectOptimalSwapChainSettings(details);

	// Simply sticking to this minimum means that we may sometimes have to wait
	// on the driver to complete internal operations before we can acquire another image to render to.
	// Therefore it is recommended to request at least one more image than the minimum
	uint32_t imageCount = details.capabilities.minImageCount + 1;

	// We should also make sure to not exceed the maximum number of images while doing this,
	// where 0 is a special value that means that there is no maximum
	if(details.capabilities.maxImageCount > 0)
		imageCount = std::min(imageCount, details.capabilities.maxImageCount);

	VkSwapchainCreateInfoKHR swapChainInfo = {};
	swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.surface = surface;
	swapChainInfo.minImageCount = imageCount;
	swapChainInfo.imageFormat = settings.format.format;
	swapChainInfo.imageColorSpace = settings.format.colorSpace;
	swapChainInfo.imageExtent = settings.extent;
	swapChainInfo.imageArrayLayers = 1;
	swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (indices.graphicsFamily.second != indices.presentFamily.second)
	{
		uint32_t queueFamilies[] = { indices.graphicsFamily.second , indices.presentFamily.second };
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainInfo.queueFamilyIndexCount = 2;
		swapChainInfo.pQueueFamilyIndices = queueFamilies;
	}
	else
	{
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainInfo.queueFamilyIndexCount = 0;
		swapChainInfo.pQueueFamilyIndices = nullptr;
	}

	swapChainInfo.preTransform = details.capabilities.currentTransform;
	swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainInfo.presentMode = settings.presentMode;
	swapChainInfo.clipped = VK_TRUE;
	swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

	result = vkCreateSwapchainKHR(device, &swapChainInfo, nullptr, &swapChain);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Can't create swapchain");

	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, nullptr);
	assert(swapChainImageCount != 0);

	swapChainImages.resize(swapChainImageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImages.data());

	swapChainImageFormat = settings.format.format;
	swapChainExtent = settings.extent;

	// Create command pool
	VkCommandPoolCreateInfo commandPoolInfo = {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.queueFamilyIndex = indices.graphicsFamily.second;
	commandPoolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
		throw std::runtime_error("Can't create command pool");

	// Create descriptor pools
	std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes = {};
	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSizes[0].descriptorCount = swapChainImageCount;
	descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSizes[1].descriptorCount = swapChainImageCount;

	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
	descriptorPoolInfo.pPoolSizes = descriptorPoolSizes.data();
	descriptorPoolInfo.maxSets = swapChainImageCount;
	descriptorPoolInfo.flags = 0; // Optional

	if (vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Can't create descriptor pool");


	// Create sync objects
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Can't create 'image available' semaphore");
		
		result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Can't create 'render finished' semaphore");

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		result = vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Can't create in flight frame fence");
	}

	RendererContext context = {};
	context.device = device;

	// Create swap chain image views
	swapChainImageViews.resize(swapChainImageCount);
	for (size_t i = 0; i < swapChainImageViews.size(); i++)
		swapChainImageViews[i] = VulkanUtils::createImage2DView(context, swapChainImages[i], swapChainImageFormat);

}

void Application::initVulkanExtensions()
{
	vkCreateDebugMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (!vkCreateDebugMessenger)
		throw std::runtime_error("Can't find vkCreateDebugUtilsMessengerEXT function");

	vkDestroyDebugMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (!vkDestroyDebugMessenger)
		throw std::runtime_error("Can't find vkDestroyDebugUtilsMessengerEXT function");
}

void Application::shutdownVulkan()
{
	vkDestroyCommandPool(device, commandPool, nullptr);
	commandPool = VK_NULL_HANDLE;

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	descriptorPool = VK_NULL_HANDLE;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}

	renderFinishedSemaphores.clear();
	imageAvailableSemaphores.clear();
	inFlightFences.clear();

	for (auto imageView : swapChainImageViews)
		vkDestroyImageView(device, imageView, nullptr);

	swapChainImageViews.clear();
	swapChainImages.clear();

	vkDestroySwapchainKHR(device, swapChain, nullptr);
	swapChain = VK_NULL_HANDLE;

	vkDestroyDevice(device, nullptr);
	device = VK_NULL_HANDLE;

	vkDestroySurfaceKHR(instance, surface, nullptr);
	surface = VK_NULL_HANDLE;

	vkDestroyDebugMessenger(instance, debugMessenger, nullptr);
	debugMessenger = VK_NULL_HANDLE;

	vkDestroyInstance(instance, nullptr);
	instance = VK_NULL_HANDLE;
}

/*
 */
int main(void)
{
	if (!glfwInit())
		return EXIT_FAILURE;

	try
	{
		Application app;
		app.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwTerminate();
	return EXIT_SUCCESS;
}
