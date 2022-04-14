#include <shaders/spirv/Compiler.h>

namespace scapes::visual::shaders
{
	Compiler *Compiler::create(ShaderILType type, foundation::io::FileSystem *file_system)
	{
		switch (type)
		{
			case ShaderILType::SPIRV: return new spirv::Compiler(file_system);
		}

		return nullptr;
	}

	void Compiler::destroy(Compiler *compiler)
	{
		delete compiler;
	}
}
