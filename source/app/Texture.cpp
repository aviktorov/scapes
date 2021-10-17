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
void resources::ResourcePipeline<Texture>::destroy(resources::ResourceManager *resource_manager, ResourceHandle<Texture> handle, render::backend::Driver *driver)
{
	Texture *texture = handle.get();
	driver->destroyTexture(texture->gpu_data);

	// TODO: use subresource pools
	delete[] texture->cpu_data;

	*texture = {};
}

bool resources::ResourcePipeline<Texture>::process(resources::ResourceManager *resource_manager, ResourceHandle<Texture> handle, const uint8_t *data, size_t size, render::backend::Driver *driver)
{
	if (stbi_info_from_memory(data, static_cast<int>(size), nullptr, nullptr, nullptr) == 0)
	{
		std::cerr << "Texture::process(): unsupported image format" << std::endl;
		return false;
	}

	void *stb_pixels = nullptr;
	size_t pixel_size = 0;
	int channels = 0;
	int default_value = 0;
	int width = 0;
	int height = 0;

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
		std::cerr << "Texture::process(): " << stbi_failure_reason() << std::endl;
		return false;
	}

	bool convert = false;
	if (channels == 3)
	{
		channels = 4;
		convert = true;
	}

	size_t image_size = width * height * channels * pixel_size;

	// TODO: use subresource pools
	uint8_t *pixels = new uint8_t[image_size];

	// As most hardware doesn't support rgb textures, convert it to rgba
	if (convert)
	{
		size_t num_pixels = width * height;
		size_t stride = pixel_size * 3;

		unsigned char *d = pixels;
		unsigned char *s = reinterpret_cast<unsigned char *>(stb_pixels);

		for (size_t i = 0; i < num_pixels; i++)
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

	Texture *result = handle.get();
	result->width = width;
	result->height = height;
	result->mip_levels = static_cast<int>(std::floor(std::log2(std::max(width, height))) + 1);
	result->layers = 1;
	result->format = deduceFormat(pixel_size, channels);

	result->cpu_data = pixels;
	result->gpu_data = driver->createTexture2D(result->width, result->height, result->mip_levels, result->format, result->cpu_data);

	driver->generateTexture2DMipmaps(result->gpu_data);

	return true;
}

/*
 */
void resources::ResourcePipeline<Texture>::create2D(ResourceHandle<Texture> handle, render::backend::Driver *driver, render::backend::Format format, int width, int height, int mip_levels, const void *data, uint32_t num_data_mipmaps)
{
	Texture *result = handle.get();

	result->format = format;
	result->width = width;
	result->height = height;
	result->mip_levels = mip_levels;
	result->layers = 1;
	result->gpu_data = driver->createTexture2D(width, height, mip_levels, format, data, num_data_mipmaps);
}

void resources::ResourcePipeline<Texture>::createCube(ResourceHandle<Texture> handle, render::backend::Driver *driver, render::backend::Format format, int size, int mip_levels, const void *data, uint32_t num_data_mipmaps)
{
	Texture *result = handle.get();

	result->format = format;
	result->width = size;
	result->height = size;
	result->mip_levels = mip_levels;
	result->layers = 6;
	result->gpu_data = driver->createTextureCube(size, mip_levels, format, data, num_data_mipmaps);
}
