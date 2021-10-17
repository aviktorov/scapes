#include "Shader.h"
#include <render/shaders/Compiler.h>

#include <iostream>

/*
 */
void resources::ResourcePipeline<Shader>::destroy(resources::ResourceHandle<Shader> handle, render::backend::Driver *driver)
{
	Shader *shader = handle.get();
	driver->destroyShader(shader->shader);

	*shader = {};
}

/*
 */
bool resources::ResourcePipeline<Shader>::import(resources::ResourceHandle<Shader> handle, const resources::URI &uri, render::backend::ShaderType type, render::backend::Driver *driver, render::shaders::Compiler *compiler)
{
	FILE *file = fopen(uri, "rb");
	if (!file)
	{
		std::cerr << "Shader::import(): can't open \"" << uri << "\" file" << std::endl;
		return false;
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8_t *data = new uint8_t[size];
	fread(data, sizeof(uint8_t), size, file);
	fclose(file);

	bool result = importFromMemory(handle, data, size, type, driver, compiler);
	delete[] data;

	return result;
}

bool resources::ResourcePipeline<Shader>::importFromMemory(resources::ResourceHandle<Shader> handle, const uint8_t *data, size_t size, render::backend::ShaderType type, render::backend::Driver *driver, render::shaders::Compiler *compiler)
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
