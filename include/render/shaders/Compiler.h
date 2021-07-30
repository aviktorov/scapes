#pragma once

#include <common/Common.h>

namespace io
{
	class IFileSystem;
}

namespace render::shaders
{
	enum class ShaderILType : uint8_t
	{
		SPIRV = 0,
		DEFAULT = SPIRV,

		MAX,
	};

	enum class ShaderType : uint8_t
	{
		// Graphics pipeline
		VERTEX = 0,
		TESSELLATION_CONTROL,
		TESSELLATION_EVALUATION,
		GEOMETRY,
		FRAGMENT,

		// Compute pipeline
		COMPUTE,

		// Raytracing pipeline
		RAY_GENERATION,
		INTERSECTION,
		ANY_HIT,
		CLOSEST_HIT,
		MISS,
		CALLABLE,

		MAX,
	};

	struct ShaderIL
	{
		ShaderType type {ShaderType::FRAGMENT};
		ShaderILType il_type {ShaderILType::DEFAULT};
		size_t bytecode_size {0};
		void *bytecode_data {nullptr};
	};

	class Compiler
	{
	public:
		static SCAPES_API Compiler *create(ShaderILType type = ShaderILType::DEFAULT, io::IFileSystem *file_system = nullptr);
		static SCAPES_API void destroy(Compiler *compiler);

		virtual ~Compiler() { }

	public:
		virtual ShaderIL *createShaderIL(
			ShaderType type,
			uint32_t size,
			const char *data,
			const char *path = nullptr
		) = 0;

		virtual void destroyShaderIL(ShaderIL *il) = 0;
	};
}
