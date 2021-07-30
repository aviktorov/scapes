#include <render/shaders/Compiler.h>
#include "render/shaders/spirv/Compiler.h"

#include <cassert>

namespace render::shaders
{
	Compiler *Compiler::create(ShaderILType type, io::IFileSystem *file_system)
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
