#pragma once

#include <scapes/visual/shaders/Compiler.h>
#include <shaderc/shaderc.h>

namespace scapes::visual::shaders
{
	class ShaderCache;
}

namespace scapes::visual::shaders::spirv
{
	class Compiler : public shaders::Compiler
	{
	public:
		Compiler(foundation::io::FileSystem *file_system);
		~Compiler() final;

		uint64_t getHash(
			ShaderType type,
			const foundation::io::URI &uri
		) override;

		ShaderIL *createShaderIL(
			ShaderType type,
			uint32_t size,
			const char *data,
			const foundation::io::URI &uri
		) override;

		void releaseShaderIL(ShaderIL *il) override;

	private:
		shaders::ShaderCache *cache {nullptr};
		foundation::io::FileSystem *file_system {nullptr};
		shaderc_compiler_t compiler {nullptr};
		shaderc_compile_options_t options {nullptr};
	};
}
