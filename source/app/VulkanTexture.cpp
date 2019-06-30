#include "VulkanTexture.h"
#include "VulkanUtils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <iostream>

/*
 */
VulkanTexture::~VulkanTexture()
{
	clearGPUData();
	clearCPUData();
}

/*
 */
bool VulkanTexture::loadFromFile(const std::string &path)
{
	// TODO: support other image formats
	stbi_uc *stb_pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

	if (!stb_pixels)
	{
		std::cerr << "VulkanTexture::loadFromFile(): " << stbi_failure_reason() << std::endl;
		return false;
	}

	mipLevels = static_cast<int>(std::floor(std::log2(std::max(width, height))) + 1);

	size_t imageSize = width * height * 4;
	if (pixels != nullptr)
		delete[] pixels;

	pixels = new unsigned char[imageSize];
	memcpy(pixels, stb_pixels, imageSize);

	stbi_image_free(stb_pixels);
	stb_pixels = nullptr;

	// Upload CPU data to GPU
	clearGPUData();
	uploadToGPU();

	// TODO: should we clear CPU data after uploading it to the GPU?

	return true;
}

/*
 */
void VulkanTexture::uploadToGPU()
{
	// TODO: support other image formats
	format = VK_FORMAT_R8G8B8A8_UNORM;

	// Pixel data will have alpha channel even if the original image doesn't
	VkDeviceSize imageSize = width * height * 4;

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
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		image,
		imageMemory
	);

	// Prepare the image for transfer
	VulkanUtils::transitionImageLayout(
		context,
		image,
		mipLevels,
		format,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
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
		format,
		VK_FILTER_LINEAR
	);

	// Prepare the image for shader access
	VulkanUtils::transitionImageLayout(
		context,
		image,
		mipLevels,
		format,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	// Destroy staging buffer
	vkDestroyBuffer(context.device, stagingBuffer, nullptr);
	vkFreeMemory(context.device, stagingBufferMemory, nullptr);

	// Create image view & sampler
	imageView = VulkanUtils::createImage2DView(context, image, mipLevels, format, VK_IMAGE_ASPECT_COLOR_BIT);
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
}

void VulkanTexture::clearCPUData()
{
	delete[] pixels;
	pixels = nullptr;

	width = height = channels = 0;
}
