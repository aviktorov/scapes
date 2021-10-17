#include "Shader.h"
#include <render/shaders/Compiler.h>

#include <iostream>

/*
 */
void resources::ResourcePipeline<Shader>::destroy(resources::ResourceManager *resource_manager, resources::ResourceHandle<Shader> handle, render::backend::Driver *driver)
{
	Shader *shader = handle.get();
	driver->destroyShader(shader->shader);

	*shader = {};
}

bool resources::ResourcePipeline<Shader>::process(resources::ResourceManager *resource_manager, resources::ResourceHandle<Shader> handle, const uint8_t *data, size_t size, render::backend::ShaderType type, render::backend::Driver *driver, render::shaders::Compiler *compiler)
{
	Shader *shader = handle.get();
	assert(shader);

	shader->type = type;
	shader->shader = nullptr;

	render::shaders::ShaderIL *il = compiler->createShaderIL(static_cast<render::shaders::ShaderType>(type), size, reinterpret_cast<const char *>(data));

	if (il != nullptr)
	{
		shader->shader = driver->createShaderFromIL(
			static_cast<render::backend::ShaderType>(il->type),
			static_cast<render::backend::ShaderILType>(il->il_type),
			il->bytecode_size,
			il->bytecode_data
		);
	}

	compiler->destroyShaderIL(il);
	return shader->shader != nullptr;
}
