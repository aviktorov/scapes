#pragma once

#include <scapes/foundation/shaders/Compiler.h>

namespace scapes::foundation::shaders::spirv
{
	struct ShaderIL : public shaders::ShaderIL
	{
	};

	class Compiler : public shaders::Compiler
	{
	public:
		Compiler(io::FileSystem *file_system)
			: file_system(file_system) {}

		shaders::ShaderIL *createShaderIL(
			ShaderType type,
			uint32_t size,
			const char *data,
			const io::URI &uri = nullptr
		) override;

		void destroyShaderIL(shaders::ShaderIL *il) override;

	private:
		io::FileSystem *file_system {nullptr};
	};
}
