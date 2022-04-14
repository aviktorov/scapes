#pragma once

#include <scapes/foundation/TypeTraits.h>
#include <scapes/foundation/resources/ResourceManager.h>

#include <scapes/visual/hardware/Device.h>
#include <scapes/visual/Fwd.h>

namespace scapes::visual
{
	/*
	 */
	struct Shader
	{
		hardware::Shader shader {SCAPES_NULL_HANDLE};
		hardware::ShaderType type {hardware::ShaderType::FRAGMENT};
		hardware::Device *device {nullptr};
		shaders::Compiler *compiler {nullptr};
	};

	template <>
	struct ::TypeTraits<Shader>
	{
		static constexpr const char *name = "scapes::visual::Shader";
	};

	template <>
	struct ::ResourceTraits<Shader>
	{
		static SCAPES_API size_t size();
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			hardware::ShaderType type,
			hardware::Device *device,
			shaders::Compiler *compiler
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
