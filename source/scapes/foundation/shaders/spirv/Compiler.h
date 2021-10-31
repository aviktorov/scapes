#pragma once

#include <scapes/foundation/shaders/Compiler.h>
#include <shaderc/shaderc.h>

namespace scapes::foundation::shaders::spirv
{
	struct ShaderIL : public shaders::ShaderIL
	{
	};

	class Compiler : public shaders::Compiler
	{
	public:
		Compiler(io::FileSystem *file_system);
		~Compiler() final;

		shaders::ShaderIL *createShaderIL(
			ShaderType type,
			uint32_t size,
			const char *data,
			const io::URI &uri = nullptr
		) override;

		void destroyShaderIL(shaders::ShaderIL *il) override;

	private:
		io::FileSystem *file_system {nullptr};
		shaderc_compiler_t compiler {nullptr};
		shaderc_compile_options_t options {nullptr};
	};
}
