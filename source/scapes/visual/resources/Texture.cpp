#include <scapes/visual/Resources.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <scapes/foundation/profiler/Profiler.h>

#include <cassert>
#include <iostream>

using namespace scapes;
using namespace scapes::visual;

/*
 */
static foundation::render::Format deduceFormat(size_t pixel_size, int channels)
{
	assert(channels > 0 && channels <= 4);

	if (pixel_size == sizeof(stbi_uc))
	{
		switch (channels)
		{
			case 1: return foundation::render::Format::R8_UNORM;
			case 2: return foundation::render::Format::R8G8_UNORM;
			case 3: return foundation::render::Format::R8G8B8_UNORM;
			case 4: return foundation::render::Format::R8G8B8A8_UNORM;
		}
	}

	if (pixel_size == sizeof(float))
	{
		switch (channels)
		{
			case 1: return foundation::render::Format::R32_SFLOAT;
			case 2: return foundation::render::Format::R32G32_SFLOAT;
			case 3: return foundation::render::Format::R32G32B32_SFLOAT;
			case 4: return foundation::render::Format::R32G32B32A32_SFLOAT;
		}
	}

	return foundation::render::Format::UNDEFINED;
}

/*
 */
size_t ResourceTraits<resources::Texture>::size()
{
	return sizeof(resources::Texture);
}

void ResourceTraits<resources::Texture>::create(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	foundation::render::Device *device
)
{
	resources::Texture *texture = reinterpret_cast<resources::Texture *>(memory);

	*texture = {};
	texture->device = device;
}

void ResourceTraits<resources::Texture>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	resources::Texture *texture = reinterpret_cast<resources::Texture *>(memory);
	foundation::render::Device *device = texture->device;

	assert(device);

	device->destroyTexture(texture->gpu_data);

	// TODO: use subresource pools
	stbi_image_free(texture->cpu_data);

	*texture = {};
}

foundation::resources::hash_t ResourceTraits<resources::Texture>::fetchHash(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	assert(file_system);

	return file_system->mtime(uri);
}

bool ResourceTraits<resources::Texture>::reload(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	assert(file_system);

	resources::Texture *texture = reinterpret_cast<resources::Texture *>(memory);
	foundation::render::Device *device = texture->device;

	assert(device);

	resources::Texture temp = {};
	temp.device = texture->device;

	foundation::io::Stream *stream = file_system->open(uri, "rb");
	if (!stream)
	{
		foundation::Log::error("ResourceTraits<Texture>::reload(): can't open \"%s\" file\n", uri.c_str());
		return false;
	}

	size_t size = static_cast<size_t>(stream->size());

	uint8_t *data = new uint8_t[size];
	stream->read(data, sizeof(uint8_t), size);
	file_system->close(stream);

	bool success = loadFromMemory(resource_manager, &temp, data, size);

	delete[] data;
	if (!success)
		return false;

	destroy(resource_manager, memory);
	*texture = temp;

	foundation::Log::message("ResourceTraits<Texture>::reload(): file \"%s\" reloaded successfully\n", uri.c_str());
	return true;
}

bool ResourceTraits<resources::Texture>::loadFromMemory(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	const uint8_t *data,
	size_t size
)
{
	SCAPES_PROFILER();

	int width = 0;
	int height = 0;
	int channels = 0;

	if (stbi_info_from_memory(data, static_cast<int>(size), &width, &height, &channels) == 0)
	{
		std::cerr << "ResourcePipeline<Texture>::loadFromMemory(): unsupported image format" << std::endl;
		return false;
	}

	void *stb_pixels = nullptr;
	size_t pixel_size = 0;

	int desired_components = (channels == 3) ? STBI_rgb_alpha : STBI_default;
	
	if (stbi_is_hdr_from_memory(data, static_cast<int>(size)))
	{
		SCAPES_PROFILER_N("ResourcePipeline<Texture>::loadFromMemory(): import_stb_hdr_image");

		stb_pixels = stbi_loadf_from_memory(data, static_cast<int>(size), &width, &height, &channels, desired_components);
		pixel_size = sizeof(float);
	}
	else
	{
		SCAPES_PROFILER_N("ResourcePipeline<Texture>::loadFromMemory(): import_stb_regular_image");

		stb_pixels = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, desired_components);
		pixel_size = sizeof(stbi_uc);
	}

	if (!stb_pixels)
	{
		std::cerr << "ResourcePipeline<Texture>::loadFromMemory(): " << stbi_failure_reason() << std::endl;
		return false;
	}

	// In case we had 3-channel texture, we ask stb to convert to
	// 4-channel texture, so we're sure about channel count at this stage
	if (channels == 3)
		channels = 4;

	size_t image_size = width * height * channels * pixel_size;

	resources::Texture *texture = reinterpret_cast<resources::Texture *>(memory);
	texture->width = width;
	texture->height = height;
	texture->mip_levels = static_cast<int>(std::floor(std::log2(std::max(width, height))) + 1);
	texture->layers = 1;
	texture->format = deduceFormat(pixel_size, channels);

	foundation::render::Device *device = texture->device;
	assert(device);

	{
		SCAPES_PROFILER_N("ResourcePipeline<Texture>::loadFromMemory(): upload_to_gpu");
		texture->cpu_data = reinterpret_cast<unsigned char*>(stb_pixels);
		texture->gpu_data = device->createTexture2D(texture->width, texture->height, texture->mip_levels, texture->format, texture->cpu_data);
	}

	{
		SCAPES_PROFILER_N("ResourcePipeline<Texture>::loadFromMemory(): generate_2d_mipmaps");
		device->generateTexture2DMipmaps(texture->gpu_data);
	}

	return true;
}
