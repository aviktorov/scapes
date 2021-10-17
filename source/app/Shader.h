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
	static void destroy(resources::ResourceManager *resource_manager, resources::ResourceHandle<Shader> handle, render::backend::Driver *driver);
	static bool process(resources::ResourceManager *resource_manager, resources::ResourceHandle<Shader> handle, const uint8_t *data, size_t size, render::backend::ShaderType type, render::backend::Driver *driver, render::shaders::Compiler *compiler);
};
