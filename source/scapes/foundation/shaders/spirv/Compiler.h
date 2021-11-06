#pragma once

#include <scapes/foundation/shaders/Compiler.h>
#include <shaderc/shaderc.h>

#include "shaders/ShaderCache.h"

namespace scapes::foundation::shaders::spirv
{
	class Compiler : public shaders::Compiler
	{
	public:
		Compiler(io::FileSystem *file_system);
		~Compiler() final;

		ShaderIL *createShaderIL(
			ShaderType type,
			uint32_t size,
			const char *data,
			const io::URI &uri = nullptr
		) override;

		void releaseShaderIL(ShaderIL *il) override;

	private:
		ShaderCache cache;
		io::FileSystem *file_system {nullptr};
		shaderc_compiler_t compiler {nullptr};
		shaderc_compile_options_t options {nullptr};
	};
}
