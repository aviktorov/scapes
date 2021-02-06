#pragma once

#include <render/backend/driver.h>

namespace render::shaders
{
	struct ShaderIL {};

	class Compiler
	{
	public:
		virtual ShaderIL *createShaderIL(
			backend::ShaderType type,
			uint32_t size,
			const char *data,
			const char *path = nullptr
		) = 0;

		virtual void destroyShaderIL(ShaderIL *il) = 0;
	};
}
