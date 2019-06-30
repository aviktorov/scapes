#pragma once

#include <vulkan/vulkan.h>

#include <string>

#include "VulkanRendererContext.h"

/*
 */
class VulkanTexture
{
public:
	VulkanTexture(const VulkanRendererContext &context)
		: context(context) { }

	~VulkanTexture();

	inline VkImage getImage() const { return image; }
	inline VkImageView getImageView() const { return imageView; }
	inline VkSampler getSampler() const { return imageSampler; }

	bool loadFromFile(const std::string &path);

	void uploadToGPU();
	void clearGPUData();
	void clearCPUData();

private:
	VulkanRendererContext context;

	unsigned char *pixels {nullptr};
	int width {0};
	int height {0};
	int channels {0};
	int mipLevels {0};

	VkFormat format {VK_FORMAT_R8G8B8A8_UNORM};

	VkImage image {VK_NULL_HANDLE};
	VkDeviceMemory imageMemory {VK_NULL_HANDLE};
	VkImageView imageView {VK_NULL_HANDLE};
	VkSampler imageSampler {VK_NULL_HANDLE};
};
