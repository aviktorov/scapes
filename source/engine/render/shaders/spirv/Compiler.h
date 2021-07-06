#pragma once

#include <render/shaders/Compiler.h>

namespace render::shaders::spirv
{
	struct ShaderIL : public shaders::ShaderIL
	{
	};

	class Compiler : public shaders::Compiler
	{
	public:
		shaders::ShaderIL *createShaderIL(
			ShaderType type,
			uint32_t size,
			const char *data,
			const char *path = nullptr
		) override;

		void destroyShaderIL(shaders::ShaderIL *il) override;
	};
}
