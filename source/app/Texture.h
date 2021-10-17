#pragma once

#include <algorithm>
#include <render/backend/driver.h>
#include <common/ResourceManager.h>

/*
 */
struct Texture
{
	int width {0};
	int height {0};
	int mip_levels {0};
	int layers {0};
	render::backend::Format format {render::backend::Format::UNDEFINED};

	unsigned char *cpu_data {nullptr};
	render::backend::Texture *gpu_data {nullptr};

	SCAPES_INLINE int getWidth(int mip) const { return std::max<int>(1, width / (1 << mip)); }
	SCAPES_INLINE int getHeight(int mip) const { return std::max<int>(1, height / (1 << mip)); }
};

template <>
struct TypeTraits<Texture>
{
	static constexpr const char *name = "Texture";
};

template <>
struct resources::ResourcePipeline<Texture>
{
	static void destroy(resources::ResourceManager *resource_manager, ResourceHandle<Texture> handle, render::backend::Driver *driver);
	static bool process(resources::ResourceManager *resource_manager, ResourceHandle<Texture> handle, const uint8_t *data, size_t size, render::backend::Driver *driver);

	// TODO: move to render module API helpers
	static void create2D(ResourceHandle<Texture> handle, render::backend::Driver *driver, render::backend::Format format, int width, int height, int num_mips, const void *data = nullptr, uint32_t num_data_mipmaps = 1);
	static void createCube(ResourceHandle<Texture> handle, render::backend::Driver *driver, render::backend::Format format, int size, int num_mips, const void *data = nullptr, uint32_t num_data_mipmaps = 1);
};
