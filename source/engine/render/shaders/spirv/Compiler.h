#pragma once

#include <render/shaders/Compiler.h>

namespace render::shaders::spirv
{
	struct ShaderIL : public shaders::ShaderIL
	{
		backend::ShaderType type {backend::ShaderType::FRAGMENT};
		size_t bytecode_size {0};
		uint32_t *bytecode_data {nullptr};
	};

	class Compiler : public shaders::Compiler
	{
	public:
		shaders::ShaderIL *createShaderIL(
			backend::ShaderType type,
			uint32_t size,
			const char *data,
			const char *path = nullptr
		) override;

		void destroyShaderIL(shaders::ShaderIL *il) override;
	};
}
