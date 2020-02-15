#pragma once

#include <volk.h>
#include <algorithm>
#include <vector>
#include <string>

class VulkanContext;

/*
 */
class VulkanTexture
{
public:
	VulkanTexture(const VulkanContext *context)
		: context(context) { }

	~VulkanTexture();

	inline VkImage getImage() const { return image; }
	inline VkImageView getImageView() const { return imageView; }
	inline VkImageView getMipImageView(int mip) const { return mipViews[mip]; }
	inline VkSampler getSampler() const { return imageSampler; }
	inline VkFormat getImageFormat() const { return imageFormat; }

	inline int getNumLayers() const { return layers; }
	inline int getNumMipLevels() const { return mipLevels; }
	inline int getWidth() const { return width; }
	inline int getWidth(int mip) const { return std::max<int>(1, width / (1 << mip)); }
	inline int getHeight() const { return height; }
	inline int getHeight(int mip) const { return std::max<int>(1, height / (1 << mip)); }

	void create2D(VkFormat format, int width, int height, int numMipLevels);
	void createCube(VkFormat format, int width, int height, int numMipLevels);

	bool loadFromFile(const char *path);

	void clearGPUData();
	void clearCPUData();

private:
	void uploadToGPU(VkFormat format, VkImageTiling tiling, size_t imageSize);

private:
	const VulkanContext *context {nullptr};

	unsigned char *pixels {nullptr};
	int width {0};
	int height {0};
	int channels {0};
	int mipLevels {0};
	int layers {0};

	VkFormat imageFormat {VK_FORMAT_UNDEFINED};

	VkImage image {VK_NULL_HANDLE};
	VkDeviceMemory imageMemory {VK_NULL_HANDLE};
	VkImageView imageView {VK_NULL_HANDLE};
	VkSampler imageSampler {VK_NULL_HANDLE};

	std::vector<VkImageView> mipViews;
};
