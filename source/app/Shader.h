#pragma once

#include <render/backend/Driver.h>
#include <common/ResourceManager.h>

namespace render::shaders
{
	class Compiler;
}

/*
 */
struct Shader
{
	render::backend::Shader *shader {nullptr};
	render::backend::ShaderType type {render::backend::ShaderType::FRAGMENT};
};

template <>
struct TypeTraits<Shader>
{
	static constexpr const char *name = "Shader";
};

template <>
struct resources::ResourcePipeline<Shader>
{
	static bool import(resources::ResourceHandle<Shader> handle, const resources::URI &uri, render::backend::ShaderType type, render::backend::Driver *driver, render::shaders::Compiler *compiler);
	static bool importFromMemory(resources::ResourceHandle<Shader> handle, const uint8_t *data, size_t size, render::backend::ShaderType type, render::backend::Driver *driver, render::shaders::Compiler *compiler);

	static void destroy(resources::ResourceHandle<Shader> handle, render::backend::Driver *driver);
};
