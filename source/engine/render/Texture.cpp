#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cassert>
#include <iostream>

namespace render
{
	/*
	 */
	static render::backend::Format deduceFormat(size_t pixelSize, int channels)
	{
		assert(channels > 0 && channels <= 4);

		if (pixelSize == sizeof(stbi_uc))
		{
			switch (channels)
			{
				case 1: return render::backend::Format::R8_UNORM;
				case 2: return render::backend::Format::R8G8_UNORM;
				case 3: return render::backend::Format::R8G8B8_UNORM;
				case 4: return render::backend::Format::R8G8B8A8_UNORM;
			}
		}

		if (pixelSize == sizeof(float))
		{
			switch (channels)
			{
				case 1: return render::backend::Format::R32_SFLOAT;
				case 2: return render::backend::Format::R32G32_SFLOAT;
				case 3: return render::backend::Format::R32G32B32_SFLOAT;
				case 4: return render::backend::Format::R32G32B32A32_SFLOAT;
			}
		}

		return render::backend::Format::UNDEFINED;
	}

	/*
	 */
	Texture::~Texture()
	{
		clearGPUData();
		clearCPUData();
	}

	/*
	 */
	void Texture::create2D(render::backend::Format format, int w, int h, int mips)
	{
		width = w;
		height = h;
		mip_levels = mips;
		layers = 1;

		texture = driver->createTexture2D(w, h, mips, format);
	}

	/*
	 */
	void Texture::createCube(render::backend::Format format, int w, int h, int mips)
	{
		width = w;
		height = h;
		mip_levels = mips;
		layers = 6;

		texture = driver->createTextureCube(w, h, mips, format);
	}

	/*
	 */
	bool Texture::import(const char *path)
	{
		if (stbi_info(path, nullptr, nullptr, nullptr) == 0)
		{
			std::cerr << "Texture::import(): unsupported image format for \"" << path << "\" file" << std::endl;
			return false;
		}

		void *stb_pixels = nullptr;
		size_t pixel_size = 0;
		int channels = 0;

		if (stbi_is_hdr(path))
		{
			stb_pixels = stbi_loadf(path, &width, &height, &channels, STBI_default);
			pixel_size = sizeof(float);
		}
		else
		{
			stb_pixels = stbi_load(path, &width, &height, &channels, STBI_default);
			pixel_size = sizeof(stbi_uc);
		}

		if (!stb_pixels)
		{
			std::cerr << "Texture::import(): " << stbi_failure_reason() << std::endl;
			return false;
		}

		layers = 1;
		mip_levels = static_cast<int>(std::floor(std::log2(std::max(width, height))) + 1);

		bool convert = false;
		if (channels == 3)
		{
			channels = 4;
			convert = true;
		}

		size_t image_size = width * height * channels * pixel_size;
		if (pixels != nullptr)
			delete[] pixels;

		pixels = new unsigned char[image_size];

		// As most hardware doesn't support rgb textures, convert it to rgba
		if (convert)
		{
			size_t numPixels = width * height;
			size_t stride = pixel_size * 3;

			unsigned char *d = pixels;
			unsigned char *s = reinterpret_cast<unsigned char *>(stb_pixels);

			for (size_t i = 0; i < numPixels; i++)
			{
				memcpy(d, s, stride);
				s += stride;
				d += stride;

				memset(d, 0, pixel_size);
				d+= pixel_size;
			}
		}
		else
			memcpy(pixels, stb_pixels, image_size);

		stbi_image_free(stb_pixels);
		stb_pixels = nullptr;

		render::backend::Format format = deduceFormat(pixel_size, channels);
		render::backend::Multisample samples = render::backend::Multisample::COUNT_1;

		// Upload CPU data to GPU
		clearGPUData();

		texture = driver->createTexture2D(width, height, mip_levels, format, samples, pixels);
		driver->generateTexture2DMipmaps(texture);

		// TODO: should we clear CPU data after uploading it to the GPU?

		return true;
	}

	void Texture::clearGPUData()
	{
		driver->destroyTexture(texture);
		texture = nullptr;
	}

	void Texture::clearCPUData()
	{
		delete[] pixels;
		pixels = nullptr;

		width = height = 0;
	}
}
