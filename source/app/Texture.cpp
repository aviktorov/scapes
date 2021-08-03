#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cassert>
#include <iostream>

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
void Texture::create2D(render::backend::Format f, int w, int h, int mips)
{
	width = w;
	height = h;
	mip_levels = mips;
	layers = 1;
	format = f;

	texture = driver->createTexture2D(w, h, mips, format);
}

/*
 */
void Texture::createCube(render::backend::Format f, int size, int mips)
{
	width = size;
	height = size;
	mip_levels = mips;
	layers = 6;
	format = f;

	texture = driver->createTextureCube(size, mips, format);
}

/*
 */
void Texture::setSamplerWrapMode(render::backend::SamplerWrapMode mode)
{
	if (!texture)
		return;

	driver->setTextureSamplerWrapMode(texture, mode);
}

/*
 */
bool Texture::import(const char *path)
{
	FILE *file = fopen(path, "rb");
	if (!file)
	{
		std::cerr << "Texture::import(): can't open \"" << path << "\" file" << std::endl;
		return false;
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8_t *data = new uint8_t[size];
	fread(data, sizeof(uint8_t), size, file);
	fclose(file);

	bool result = importFromMemory(data, size);
	delete[] data;

	return result;
}

bool Texture::importFromMemory(const uint8_t *data, size_t size)
{
	if (stbi_info_from_memory(data, static_cast<int>(size), nullptr, nullptr, nullptr) == 0)
	{
		std::cerr << "Texture::importFromMemory(): unsupported image format" << std::endl;
		return false;
	}

	void *stb_pixels = nullptr;
	size_t pixel_size = 0;
	int channels = 0;
	int default_value = 0;

	if (stbi_is_hdr_from_memory(data, static_cast<int>(size)))
	{
		stb_pixels = stbi_loadf_from_memory(data, static_cast<int>(size), &width, &height, &channels, STBI_default);
		pixel_size = sizeof(float);
		default_value = 0;
	}
	else
	{
		stb_pixels = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, STBI_default);
		pixel_size = sizeof(stbi_uc);
		default_value = 255;
	}

	if (!stb_pixels)
	{
		std::cerr << "Texture::importFromMemory(): " << stbi_failure_reason() << std::endl;
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

			memset(d, default_value, pixel_size);
			d+= pixel_size;
		}
	}
	else
		memcpy(pixels, stb_pixels, image_size);

	stbi_image_free(stb_pixels);
	stb_pixels = nullptr;

	format = deduceFormat(pixel_size, channels);

	// Upload CPU data to GPU
	clearGPUData();

	texture = driver->createTexture2D(width, height, mip_levels, format, pixels);
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
