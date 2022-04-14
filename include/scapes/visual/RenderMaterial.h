#pragma once

#include <scapes/foundation/TypeTraits.h>
#include <scapes/foundation/resources/ResourceManager.h>

#include <scapes/visual/Fwd.h>

namespace scapes::visual
{
	/*
	 */
	struct RenderMaterial
	{
		TextureHandle albedo;
		TextureHandle normal;
		TextureHandle roughness;
		TextureHandle metalness;
		hardware::BindSet bindings {SCAPES_NULL_HANDLE};
		hardware::Device *device {nullptr};
	};

	template <>
	struct ::TypeTraits<RenderMaterial>
	{
		static constexpr const char *name = "scapes::visual::RenderMaterial";
	};

	template <>
	struct ::ResourceTraits<RenderMaterial>
	{
		static SCAPES_API size_t size();
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			TextureHandle albedo,
			TextureHandle normal,
			TextureHandle roughness,
			TextureHandle metalness,
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
