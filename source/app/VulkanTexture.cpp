#include "VulkanTexture.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cassert>
#include <algorithm>
#include <iostream>

static VkFormat deduceFormat(size_t pixelSize, int channels)
{
	assert(channels > 0 && channels <= 4);

	if (pixelSize == sizeof(stbi_uc))
	{
		switch (channels)
		{
			case 1: return VK_FORMAT_R8_UNORM;
			case 2: return VK_FORMAT_R8G8_UNORM;
			case 3: return VK_FORMAT_R8G8B8_UNORM;
			case 4: return VK_FORMAT_R8G8B8A8_UNORM;
		}
	}

	if (pixelSize == sizeof(float))
	{
		switch (channels)
		{
			case 1: return VK_FORMAT_R32_SFLOAT;
			case 2: return VK_FORMAT_R32G32_SFLOAT;
			case 3: return VK_FORMAT_R32G32B32_SFLOAT;
			case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		}
	}

	return VK_FORMAT_UNDEFINED;
}

static int deduceChannels(VkFormat format)
{
	assert(format != VK_FORMAT_UNDEFINED);

	switch(format)
	{
		case VK_FORMAT_R8_UNORM: return 1;
		case VK_FORMAT_R8G8_UNORM: return 2;
		case VK_FORMAT_R8G8B8_UNORM: return 3;
		case VK_FORMAT_R8G8B8A8_UNORM: return 4;
		case VK_FORMAT_R16_SFLOAT: return 1;
		case VK_FORMAT_R16G16_SFLOAT: return 2;
		case VK_FORMAT_R16G16B16_SFLOAT: return 3;
		case VK_FORMAT_R16G16B16A16_SFLOAT: return 4;
		case VK_FORMAT_R32_SFLOAT: return 1;
		case VK_FORMAT_R32G32_SFLOAT: return 2;
		case VK_FORMAT_R32G32B32_SFLOAT: return 3;
		case VK_FORMAT_R32G32B32A32_SFLOAT: return 4;
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
void VulkanTexture::create2D(VkFormat format, int w, int h, int mips)
{
	width = w;
	height = h;
	mipLevels = mips;
	layers = 1;
	imageFormat = format;

	channels = deduceChannels(format);
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

	VulkanUtils::createImage2D(
		context,
		width,
		height,
		mipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		imageFormat,
		tiling,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
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
	imageView = VulkanUtils::createImageView(
		context,
		image,
		imageFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0, mipLevels,
		0, layers
	);
	imageSampler = VulkanUtils::createSampler(context, mipLevels);
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
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

	VulkanUtils::createImageCube(
		context,
		width,
		height,
		mipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		imageFormat,
		tiling,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
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
	imageView = VulkanUtils::createImageView(
		context,
		image,
		imageFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		0, mipLevels,
		0, layers
	);
	imageSampler = VulkanUtils::createSampler(context, mipLevels);
}

/*
 */
bool VulkanTexture::loadFromFile(const char *path)
{
	if (stbi_info(path, nullptr, nullptr, nullptr) == 0)
	{
		std::cerr << "VulkanTexture::loadFromFile(): unsupported image format for \"" << path << "\" file" << std::endl;
		return false;
	}

	void *stbPixels = nullptr;
	size_t pixelSize = 0;

	if (stbi_is_hdr(path))
	{
		stbPixels = stbi_loadf(path, &width, &height, &channels, STBI_default);
		pixelSize = sizeof(float);
	}
	else
	{
		stbPixels = stbi_load(path, &width, &height, &channels, STBI_default);
		pixelSize = sizeof(stbi_uc);
	}

	if (!stbPixels)
	{
		std::cerr << "VulkanTexture::loadFromFile(): " << stbi_failure_reason() << std::endl;
		return false;
	}

	layers = 1;
	mipLevels = static_cast<int>(std::floor(std::log2(std::max(width, height))) + 1);

	bool convert = false;
	if (channels == 3)
	{
		channels = 4;
		convert = true;
	}

	size_t imageSize = width * height * channels * pixelSize;
	if (pixels != nullptr)
		delete[] pixels;

	pixels = new unsigned char[imageSize];

	// As most hardware doesn't support rgb textures, convert it to rgba
	if (convert)
	{
		size_t numPixels = width * height;
		size_t stride = pixelSize * 3;

		unsigned char *d = pixels;
		unsigned char *s = reinterpret_cast<unsigned char *>(stbPixels);

		for (int i = 0; i < numPixels; i++)
		{
			memcpy(d, s, stride);
			s += stride;
			d += stride;

			memset(d, 0, pixelSize);
			d+= pixelSize;
		}
	}
	else
		memcpy(pixels, stbPixels, imageSize);

	stbi_image_free(stbPixels);
	stbPixels = nullptr;

	VkFormat format = deduceFormat(pixelSize, channels);

	// Upload CPU data to GPU
	clearGPUData();
	uploadToGPU(format, VK_IMAGE_TILING_OPTIMAL, imageSize);

	// TODO: should we clear CPU data after uploading it to the GPU?

	return true;
}

/*
 */
void VulkanTexture::uploadToGPU(VkFormat format, VkImageTiling tiling, size_t imageSize)
{
	imageFormat = format;

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

	VulkanUtils::createBuffer(
		context,
		static_cast<VkDeviceSize>(imageSize),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	// Fill staging buffer
	void *data = nullptr;
	vkMapMemory(context->getDevice(), stagingBufferMemory, 0, static_cast<VkDeviceSize>(imageSize), 0, &data);
	memcpy(data, pixels, imageSize);
	vkUnmapMemory(context->getDevice(), stagingBufferMemory);

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
	vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(context->getDevice(), stagingBufferMemory, nullptr);

	// Create image view & sampler
	imageView = VulkanUtils::createImageView(
		context,
		image,
		imageFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0, mipLevels,
		0, layers
	);

	imageSampler = VulkanUtils::createSampler(context, mipLevels);
}

void VulkanTexture::clearGPUData()
{
	vkDestroySampler(context->getDevice(), imageSampler, nullptr);
	imageSampler = nullptr;

	vkDestroyImageView(context->getDevice(), imageView, nullptr);
	imageView = nullptr;

	vkDestroyImage(context->getDevice(), image, nullptr);
	image = nullptr;

	vkFreeMemory(context->getDevice(), imageMemory, nullptr);
	imageMemory = nullptr;

	imageFormat = VK_FORMAT_UNDEFINED;
}

void VulkanTexture::clearCPUData()
{
	delete[] pixels;
	pixels = nullptr;

	width = height = channels = 0;
}
