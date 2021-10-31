#include <scapes/visual/Resources.h>

#include <Tracy.hpp>

using namespace scapes;
using namespace scapes::visual;

/*
 */
void ResourcePipeline<resources::Shader>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	ShaderHandle handle,
	foundation::render::Device *device
)
{
	resources::Shader *shader = handle.get();
	device->destroyShader(shader->shader);

	*shader = {};
}

bool ResourcePipeline<resources::Shader>::process(
	foundation::resources::ResourceManager *resource_manager,
	ShaderHandle handle,
	const uint8_t *data,
	size_t size,
	foundation::render::ShaderType type,
	foundation::render::Device *device,
	foundation::shaders::Compiler *compiler
)
{
	ZoneScopedN("ResourcePipeline<Shader>::process");

	resources::Shader *shader = handle.get();
	assert(shader);

	shader->type = type;
	shader->shader = nullptr;

	foundation::shaders::ShaderIL *il = nullptr;
	
	{
		ZoneScopedN("ResourcePipeline<Shader>::compile_to_spirv");
		il = compiler->createShaderIL(static_cast<foundation::shaders::ShaderType>(type), static_cast<uint32_t>(size), reinterpret_cast<const char *>(data));
	}

	if (il != nullptr)
	{
		ZoneScopedN("ResourcePipeline<Shader>::create_from_spirv");
		shader->shader = device->createShaderFromIL(
			static_cast<foundation::render::ShaderType>(il->type),
			static_cast<foundation::render::ShaderILType>(il->il_type),
			il->bytecode_size,
			il->bytecode_data
		);
	}

	compiler->destroyShaderIL(il);
	return shader->shader != nullptr;
}
