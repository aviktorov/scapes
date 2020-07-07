#pragma once

#include <algorithm>
#include <render/backend/driver.h>

namespace render
{
	/*
	 */
	class Texture
	{
	public:
		Texture(backend::Driver *driver)
			: driver(driver) { }

		~Texture();

		inline int getNumLayers() const { return layers; }
		inline int getNumMipLevels() const { return mip_levels; }
		inline int getWidth() const { return width; }
		inline int getWidth(int mip) const { return std::max<int>(1, width / (1 << mip)); }
		inline int getHeight() const { return height; }
		inline int getHeight(int mip) const { return std::max<int>(1, height / (1 << mip)); }

		inline const backend::Texture *getBackend() const { return texture; }

		void create2D(backend::Format format, int width, int height, int num_mips);
		void createCube(backend::Format format, int width, int height, int num_mips);

		bool import(const char *path);

		void clearGPUData();
		void clearCPUData();

	private:
		backend::Driver *driver {nullptr};

		unsigned char *pixels {nullptr};
		int width {0};
		int height {0};
		int mip_levels {0};
		int layers {0};

		backend::Texture *texture {nullptr};
	};
}
