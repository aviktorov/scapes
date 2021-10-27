#include <scapes/visual/Resources.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <Tracy.hpp>

#include <cassert>
#include <iostream>

using namespace scapes::visual;

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
void ResourcePipeline<resources::Texture>::destroy(ResourceManager *resource_manager, TextureHandle handle, render::backend::Driver *driver)
{
	resources::Texture *texture = handle.get();

	driver->destroyTexture(texture->gpu_data);

	// TODO: use subresource pools
	stbi_image_free(texture->cpu_data);

	*texture = {};
}

bool ResourcePipeline<resources::Texture>::process(ResourceManager *resource_manager, TextureHandle handle, const uint8_t *data, size_t size, render::backend::Driver *driver)
{
	ZoneScopedN("ResourcePipeline<Texture>::process");

	int width = 0;
	int height = 0;
	int channels = 0;

	if (stbi_info_from_memory(data, static_cast<int>(size), &width, &height, &channels) == 0)
	{
		std::cerr << "ResourcePipeline<Texture>::process(): unsupported image format" << std::endl;
		return false;
	}

	void *stb_pixels = nullptr;
	size_t pixel_size = 0;

	int desired_components = (channels == 3) ? STBI_rgb_alpha : STBI_default;
	
	if (stbi_is_hdr_from_memory(data, static_cast<int>(size)))
	{
		ZoneScopedN("ResourcePipeline<Texture>::import_stb_hdr_image");

		stb_pixels = stbi_loadf_from_memory(data, static_cast<int>(size), &width, &height, &channels, desired_components);
		pixel_size = sizeof(float);
	}
	else
	{
		ZoneScopedN("ResourcePipeline<Texture>::import_stb_regular_image");

		stb_pixels = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, desired_components);
		pixel_size = sizeof(stbi_uc);
	}

	if (!stb_pixels)
	{
		std::cerr << "ResourcePipeline<Texture>::process(): " << stbi_failure_reason() << std::endl;
		return false;
	}

	// In case we had 3-channel texture, we ask stb to convert to
	// 4-channel texture, so we're sure about channel count at this stage
	if (channels == 3)
		channels = 4;

	size_t image_size = width * height * channels * pixel_size;

	resources::Texture *result = handle.get();
	result->width = width;
	result->height = height;
	result->mip_levels = static_cast<int>(std::floor(std::log2(std::max(width, height))) + 1);
	result->layers = 1;
	result->format = deduceFormat(pixel_size, channels);

	{
		ZoneScopedN("ResourcePipeline<Texture>::upload_to_gpu");
		result->cpu_data = reinterpret_cast<unsigned char*>(stb_pixels);
		result->gpu_data = driver->createTexture2D(result->width, result->height, result->mip_levels, result->format, result->cpu_data);
	}

	{
		ZoneScopedN("ResourcePipeline<Texture>::generate_2d_mipmaps");
		driver->generateTexture2DMipmaps(result->gpu_data);
	}

	return true;
}
