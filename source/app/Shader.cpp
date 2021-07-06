#include "Shader.h"
#include <render/shaders/Compiler.h>

#include <fstream>
#include <iostream>
#include <vector>

/*
 */
Shader::~Shader()
{
	clear();
}

/*
 */
bool Shader::compileFromFile(render::backend::ShaderType shader_type, const char *file_path)
{
	std::ifstream file(file_path, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		std::cerr << "Shader::compileFromFile(): can't load shader at \"" << file_path << "\"" << std::endl;
		return false;
	}

	size_t size = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(size);

	file.seekg(0);
	file.read(buffer.data(), size);
	file.close();

	path = file_path;
	type = shader_type;

	return compile(shader_type, static_cast<uint32_t>(buffer.size()), buffer.data(), file_path);
}

bool Shader::compileFromMemory(render::backend::ShaderType shader_type, uint32_t size, const char *data)
{
	path.clear();
	type = type;

	return compile(shader_type, size, data, nullptr);
}

/*
 */
bool Shader::reload()
{
	if (path.empty())
		return false;

	return compileFromFile(type, path.c_str());
}

void Shader::clear()
{
	driver->destroyShader(shader);
	shader = nullptr;
}

/*
 */
bool Shader::compile(render::backend::ShaderType type, uint32_t size, const char *data, const char *path)
{
	driver->destroyShader(shader);
	shader = nullptr;

	render::shaders::ShaderIL *il = compiler->createShaderIL(static_cast<render::shaders::ShaderType>(type), size, data, path);

	if (il != nullptr)
		shader = driver->createShaderFromIL(
			static_cast<render::backend::ShaderType>(il->type),
			static_cast<render::backend::ShaderILType>(il->il_type),
			il->bytecode_size,
			il->bytecode_data
		);

	compiler->destroyShaderIL(il);

	return shader != nullptr;
}
