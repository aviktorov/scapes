// TODO: remove Vulkan dependencies
#pragma once

#include <volk.h>

#include <algorithm>
#include <vector>
#include <string>

#include <render/backend/driver.h>

/*
 */
class VulkanTexture
{
public:
	VulkanTexture(render::backend::Driver *driver)
		: driver(driver) { }

	~VulkanTexture();

	VkImage getImage() const;
	VkFormat getImageFormat() const;
	VkImageView getImageView() const;
	VkSampler getSampler() const;

	inline int getNumLayers() const { return layers; }
	inline int getNumMipLevels() const { return mip_levels; }
	inline int getWidth() const { return width; }
	inline int getWidth(int mip) const { return std::max<int>(1, width / (1 << mip)); }
	inline int getHeight() const { return height; }
	inline int getHeight(int mip) const { return std::max<int>(1, height / (1 << mip)); }

	inline const render::backend::Texture *getBackend() const { return texture; }

	void create2D(render::backend::Format format, int width, int height, int num_mips);
	void createCube(render::backend::Format format, int width, int height, int num_mips);

	bool import(const char *path);

	void clearGPUData();
	void clearCPUData();

private:
	render::backend::Driver *driver {nullptr};

	unsigned char *pixels {nullptr};
	int width {0};
	int height {0};
	int mip_levels {0};
	int layers {0};

	render::backend::Texture *texture {nullptr};
};
