#include <render/shaders/Compiler.h>
#include "render/shaders/spirv/Compiler.h"

#include <cassert>

namespace render::shaders
{
	Compiler *Compiler::create(ShaderILType type)
	{
		switch (type)
		{
			case ShaderILType::SPIRV: return new spirv::Compiler();
		}

		return nullptr;
	}
}
