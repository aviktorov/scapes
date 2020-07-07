#include "Shader.h"

#include <fstream>
#include <iostream>
#include <vector>

namespace render
{
	/*
	 */
	Shader::~Shader()
	{
		clear();
	}

	/*
	 */
	bool Shader::compileFromFile(const char *file_path, render::backend::ShaderType shader_type)
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

		shader = driver->createShaderFromSource(shader_type, static_cast<uint32_t>(buffer.size()), buffer.data(), file_path);

		path = file_path;
		type = shader_type;

		return shader != nullptr;
	}

	bool Shader::reload()
	{
		return compileFromFile(path.c_str(), type);
	}

	void Shader::clear()
	{
		driver->destroyShader(shader);
		shader = nullptr;
	}
}
