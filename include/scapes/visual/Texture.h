#pragma once

#include <scapes/foundation/TypeTraits.h>
#include <scapes/foundation/resources/ResourceManager.h>

#include <scapes/visual/hardware/Device.h>
#include <scapes/visual/Fwd.h>

namespace scapes::visual
{
	/*
	 */
	struct Texture
	{
		uint32_t width {0};
		uint32_t height {0};
		uint32_t depth {0};
		uint32_t mip_levels {0};
		uint32_t layers {0};
		hardware::Format format {hardware::Format::UNDEFINED};

		unsigned char *cpu_data {nullptr};
		hardware::Texture gpu_data {SCAPES_NULL_HANDLE};
		hardware::Device *device {nullptr};
	};

	template <>
	struct ::TypeTraits<Texture>
	{
		static constexpr const char *name = "scapes::visual::Texture";
	};

	template <>
	struct ::ResourceTraits<Texture>
	{
		static SCAPES_API size_t size();
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			hardware::Device *device
		);
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			hardware::Device *device,
			hardware::Format format,
			uint32_t width,
			uint32_t height,
			uint32_t mip_levels = 1,
			uint32_t layers = 1
		);
		static SCAPES_API void destroy(
			foundation::resources::ResourceManager *resource_manager,
			void *memory
		);
		static SCAPES_API foundation::resources::hash_t fetchHash(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API bool reload(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API bool loadFromMemory(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			const uint8_t *data,
			size_t size
		);
	};
}
