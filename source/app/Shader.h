#pragma once

#include <string>
#include <render/backend/driver.h>

namespace render::shaders
{
	class Compiler;
}

/*
 */
class Shader
{
public:
	Shader(render::backend::Driver *driver, render::shaders::Compiler *compiler)
		: driver(driver), compiler(compiler) { }

	~Shader();

	bool compileFromFile(render::backend::ShaderType type, const char *path);
	bool compileFromMemory(render::backend::ShaderType type, uint32_t size, const char *data);
	bool reload();
	void clear();

	inline render::backend::Shader *getBackend() const { return shader; }

private:
	bool compile(render::backend::ShaderType type, uint32_t size, const char *data, const char *path);

private:
	render::backend::Driver *driver {nullptr};
	render::backend::Shader *shader {nullptr};
	render::backend::ShaderType type {render::backend::ShaderType::FRAGMENT};
	render::shaders::Compiler *compiler {nullptr};

	std::string path;
};
