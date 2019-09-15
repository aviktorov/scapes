#include "VulkanTexture.h"
#include "VulkanUtils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <iostream>

static int deduceChannels(VkFormat format)
{
	switch(format)
	{
		case VK_FORMAT_R32_SFLOAT: return 1;
		case VK_FORMAT_R32G32_SFLOAT: return 2;
		case VK_FORMAT_R32G32B32_SFLOAT: return 3;
		case VK_FORMAT_R32G32B32A32_SFLOAT: return 4;
		case VK_FORMAT_R8G8B8A8_UNORM: return 4;
		default: throw std::runtime_error("Format is not supported");
	}
}

static size_t deducePixelSize(VkFormat format)
{
	switch(format)
	{
		case VK_FORMAT_R32_SFLOAT: return 4;
		case VK_FORMAT_R32G32_SFLOAT: return 8;
		case VK_FORMAT_R32G32B32_SFLOAT: return 12;
		case VK_FORMAT_R32G32B32A32_SFLOAT: return 16;
		case VK_FORMAT_R8G8B8A8_UNORM: return 4;
		default: throw std::runtime_error("Format is not supported");
	}
}

static VkImageTiling deduceTiling(VkFormat format)
{
	switch(format)
	{
		case VK_FORMAT_R32_SFLOAT: return VK_IMAGE_TILING_LINEAR;
		case VK_FORMAT_R32G32_SFLOAT: return VK_IMAGE_TILING_LINEAR;
		case VK_FORMAT_R32G32B32_SFLOAT: return  VK_IMAGE_TILING_LINEAR;
		case VK_FORMAT_R32G32B32A32_SFLOAT: return  VK_IMAGE_TILING_LINEAR;
		case VK_FORMAT_R8G8B8A8_UNORM: return VK_IMAGE_TILING_OPTIMAL;
		default: throw std::runtime_error("Format is not supported");
	}
}


/*
 */
VulkanTexture::~VulkanTexture()
{
	clearGPUData();
	clearCPUData();
}

/*
 */
void VulkanTexture::createCube(VkFormat format, int w, int h, int mips)
{
	width = w;
	height = h;
	mipLevels = mips;
	layers = 6;
	imageFormat = format;

	channels = deduceChannels(format);
	VkImageTiling tiling = deduceTiling(format);

	VulkanUtils::createImageCube(
		context,
		width,
		height,
		mipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		imageFormat,
		tiling,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		image,
		imageMemory
	);

	// Prepare the image for shader access
	VulkanUtils::transitionImageLayout(
		context,
		image,
		imageFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		0,
		mipLevels,
		0,
		layers
	);

	// Create image view & sampler
	imageView = VulkanUtils::createImageView(context, image, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, layers);
	imageSampler = VulkanUtils::createSampler(context, mipLevels);
}

/*
 */
bool VulkanTexture::loadHDRFromFile(const std::string &path)
{
	stbi_set_flip_vertically_on_load(true);
	float *stb_pixels = stbi_loadf(path.c_str(), &width, &height, &channels, 0);

	if (!stb_pixels)
	{
		std::cerr << "VulkanTexture::loadHDRFromFile(): " << stbi_failure_reason() << std::endl;
		return false;
	}

	mipLevels = 1;
	layers = 1;

	size_t pixelSize = channels * sizeof(float);
	size_t imageSize = width * height * pixelSize;
	if (pixels != nullptr)
		delete[] pixels;

	pixels = new unsigned char[imageSize];
	memcpy(pixels, stb_pixels, imageSize);

	stbi_image_free(stb_pixels);
	stb_pixels = nullptr;

	// Upload CPU data to GPU
	clearGPUData();

	VkFormat format = VK_FORMAT_UNDEFINED;
	switch(channels)
	{
		case 1: format = VK_FORMAT_R32_SFLOAT;
		case 2: format = VK_FORMAT_R32G32_SFLOAT;
		case 3: format = VK_FORMAT_R32G32B32_SFLOAT;
	}
	uploadToGPU(format, VK_IMAGE_TILING_LINEAR, pixelSize);

	return true;
}

/*
 */
bool VulkanTexture::loadFromFile(const std::string &path)
{
	// TODO: support other image formats
	stbi_uc *stb_pixels = stbi_load(path.c_str(), &width, &height, nullptr, STBI_rgb_alpha);
	channels = 4;

	if (!stb_pixels)
	{
		std::cerr << "VulkanTexture::loadFromFile(): " << stbi_failure_reason() << std::endl;
		return false;
	}

	mipLevels = static_cast<int>(std::floor(std::log2(std::max(width, height))) + 1);
	layers = 1;

	size_t pixelSize = channels * sizeof(stbi_uc);
	size_t imageSize = width * height * pixelSize;
	if (pixels != nullptr)
		delete[] pixels;

	pixels = new unsigned char[imageSize];
	memcpy(pixels, stb_pixels, imageSize);

	stbi_image_free(stb_pixels);
	stb_pixels = nullptr;

	// Upload CPU data to GPU
	clearGPUData();
	uploadToGPU(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, pixelSize);

	// TODO: should we clear CPU data after uploading it to the GPU?

	return true;
}

/*
 */
void VulkanTexture::uploadToGPU(VkFormat format, VkImageTiling tiling, size_t pixelSize)
{
	imageFormat = format;

	VkDeviceSize imageSize = width * height * pixelSize;

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

	VulkanUtils::createImage2D(
		context,
		width,
		height,
		mipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		imageFormat,
		tiling,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		image,
		imageMemory
	);

	// Prepare the image for transfer
	VulkanUtils::transitionImageLayout(
		context,
		image,
		imageFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		0,
		mipLevels
	);

	// Copy to the image memory on GPU
	VulkanUtils::copyBufferToImage(
		context,
		stagingBuffer,
		image,
		width,
		height
	);

	// Generate mipmaps on GPU with linear filtering
	VulkanUtils::generateImage2DMipmaps(
		context,
		image,
		width,
		height,
		mipLevels,
		imageFormat,
		VK_FILTER_LINEAR
	);

	// Prepare the image for shader access
	VulkanUtils::transitionImageLayout(
		context,
		image,
		imageFormat,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		0,
		mipLevels
	);

	// Destroy staging buffer
	vkDestroyBuffer(context.device, stagingBuffer, nullptr);
	vkFreeMemory(context.device, stagingBufferMemory, nullptr);

	// Create image view & sampler
	imageView = VulkanUtils::createImageView(context, image, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, layers);
	imageSampler = VulkanUtils::createSampler(context, mipLevels);
}

void VulkanTexture::clearGPUData()
{
	vkDestroySampler(context.device, imageSampler, nullptr);
	imageSampler = nullptr;

	vkDestroyImageView(context.device, imageView, nullptr);
	imageView = nullptr;

	vkDestroyImage(context.device, image, nullptr);
	image = nullptr;

	vkFreeMemory(context.device, imageMemory, nullptr);
	imageMemory = nullptr;

	imageFormat = VK_FORMAT_UNDEFINED;
}

void VulkanTexture::clearCPUData()
{
	delete[] pixels;
	pixels = nullptr;

	width = height = channels = 0;
}
