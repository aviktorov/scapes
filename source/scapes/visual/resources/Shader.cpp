#include <scapes/visual/Resources.h>

#include <Tracy.hpp>

using namespace scapes::visual;

/*
 */
void ResourcePipeline<resources::Shader>::destroy(ResourceManager *resource_manager, ShaderHandle handle, render::backend::Driver *driver)
{
	resources::Shader *shader = handle.get();
	driver->destroyShader(shader->shader);

	*shader = {};
}

bool ResourcePipeline<resources::Shader>::process(ResourceManager *resource_manager, ShaderHandle handle, const uint8_t *data, size_t size, render::backend::ShaderType type, render::backend::Driver *driver, render::shaders::Compiler *compiler)
{
	ZoneScopedN("ResourcePipeline<Shader>::process");

	resources::Shader *shader = handle.get();
	assert(shader);

	shader->type = type;
	shader->shader = nullptr;

	render::shaders::ShaderIL *il = nullptr;
	
	{
		ZoneScopedN("ResourcePipeline<Shader>::compile_to_spirv");
		il = compiler->createShaderIL(static_cast<render::shaders::ShaderType>(type), static_cast<uint32_t>(size), reinterpret_cast<const char *>(data));
	}

	if (il != nullptr)
	{
		ZoneScopedN("ResourcePipeline<Shader>::create_from_spirv");
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
