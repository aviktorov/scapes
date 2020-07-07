#pragma once

#include <string>
#include <render/backend/driver.h>

namespace render
{
	/*
	 */
	class Shader
	{
	public:
		Shader(backend::Driver *driver)
			: driver(driver) { }

		~Shader();

		bool compileFromFile(const char *path, backend::ShaderType type);
		bool reload();
		void clear();

		inline backend::Shader *getBackend() const { return shader; }

	private:
		backend::Driver *driver {nullptr};
		backend::Shader *shader {nullptr};
		backend::ShaderType type {backend::ShaderType::FRAGMENT};

		std::string path;
	};
}
