#pragma once

#include <string>
#include <render/backend/driver.h>

namespace render::shaders
{
	class Compiler;
}

namespace render
{
	/*
	 */
	class Shader
	{
	public:
		Shader(backend::Driver *driver, shaders::Compiler *compiler)
			: driver(driver), compiler(compiler) { }

		~Shader();

		bool compileFromFile(backend::ShaderType type, const char *path);
		bool compileFromMemory(backend::ShaderType type, uint32_t size, const char *data);
		bool reload();
		void clear();

		inline backend::Shader *getBackend() const { return shader; }

	private:
		bool compile(backend::ShaderType type, uint32_t size, const char *data, const char *path);

	private:
		backend::Driver *driver {nullptr};
		backend::Shader *shader {nullptr};
		backend::ShaderType type {backend::ShaderType::FRAGMENT};
		shaders::Compiler *compiler {nullptr};

		std::string path;
	};
}
