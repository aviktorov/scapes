#pragma once

#include <algorithm>
#include <render/backend/driver.h>

/*
 */
class Texture
{
public:
	Texture(render::backend::Driver *driver)
		: driver(driver) { }

	~Texture();

	inline int getNumLayers() const { return layers; }
	inline int getNumMipLevels() const { return mip_levels; }
	inline int getWidth() const { return width; }
	inline int getWidth(int mip) const { return std::max<int>(1, width / (1 << mip)); }
	inline int getHeight() const { return height; }
	inline int getHeight(int mip) const { return std::max<int>(1, height / (1 << mip)); }
	inline render::backend::Format getFormat() const { return format; }

	inline const render::backend::Texture *getBackend() const { return texture; }

	void create2D(render::backend::Format format, int width, int height, int num_mips);
	void createCube(render::backend::Format format, int size, int num_mips);

	void setSamplerWrapMode(render::backend::SamplerWrapMode mode);

	bool import(const char *path);
	bool importFromMemory(const uint8_t *data, size_t size);

	void clearGPUData();
	void clearCPUData();

private:
	render::backend::Driver *driver {nullptr};

	unsigned char *pixels {nullptr};
	int width {0};
	int height {0};
	int mip_levels {0};
	int layers {0};

	render::backend::Format format {render::backend::Format::UNDEFINED};
	render::backend::Texture *texture {nullptr};
};
