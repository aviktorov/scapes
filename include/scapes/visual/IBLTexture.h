#pragma once

#include <scapes/foundation/TypeTraits.h>
#include <scapes/foundation/resources/ResourceManager.h>

#include <scapes/visual/Fwd.h>

namespace scapes::visual
{
	/*
	 */
	struct IBLTexture
	{
		// TODO: move to skylight
		TextureHandle baked_brdf;

		hardware::Texture prefiltered_specular_cubemap {SCAPES_NULL_HANDLE};
		hardware::Texture diffuse_irradiance_cubemap {SCAPES_NULL_HANDLE};

		hardware::BindSet bindings {SCAPES_NULL_HANDLE};
		hardware::Device *device {nullptr};
	};

	template <>
	struct ::TypeTraits<IBLTexture>
	{
		static constexpr const char *name = "scapes::visual::IBLTexture";
	};

	template <>
	struct ::ResourceTraits<IBLTexture>
	{
		static SCAPES_API size_t size();
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			hardware::Device *device
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
		static SCAPES_API void destroy(
			foundation::resources::ResourceManager *resource_manager,
			void *memory
		);
	};
}
