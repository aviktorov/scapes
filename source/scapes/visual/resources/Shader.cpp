#include <scapes/visual/Resources.h>

#include <scapes/foundation/profiler/Profiler.h>

using namespace scapes;
using namespace scapes::visual;

/*
 */
size_t ResourceTraits<resources::Shader>::size()
{
	return sizeof(resources::Shader);
}

void ResourceTraits<resources::Shader>::create(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	foundation::render::ShaderType type,
	foundation::render::Device *device,
	foundation::shaders::Compiler *compiler
)
{
	resources::Shader *shader = reinterpret_cast<resources::Shader *>(memory);

	*shader = {};
	shader->type = type;
	shader->device = device;
	shader->compiler = compiler;
}

void ResourceTraits<resources::Shader>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	resources::Shader *shader = reinterpret_cast<resources::Shader *>(memory);
	foundation::render::Device *device = shader->device;

	assert(device);

	device->destroyShader(shader->shader);

	*shader = {};
}

foundation::resources::hash_t ResourceTraits<resources::Shader>::fetchHash(
	foundation::resources::ResourceManager *resource_manager,
	const foundation::io::URI &uri
)
{
	return 0;
}

bool ResourceTraits<resources::Shader>::reload(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	const foundation::io::URI &uri
)
{
	return false;
}

bool ResourceTraits<resources::Shader>::loadFromMemory(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	const uint8_t *data,
	size_t size
)
{
	SCAPES_PROFILER();

	resources::Shader *shader = reinterpret_cast<resources::Shader *>(memory);
	assert(shader);

	foundation::render::Device *device = shader->device;
	assert(device);

	foundation::shaders::Compiler *compiler = shader->compiler;
	assert(compiler);

	foundation::shaders::ShaderIL *il = nullptr;
	
	{
		SCAPES_PROFILER_N("ResourcePipeline<Shader>::loadFromMemory(): compile_to_spirv");
		il = compiler->createShaderIL(static_cast<foundation::shaders::ShaderType>(shader->type), static_cast<uint32_t>(size), reinterpret_cast<const char *>(data));
	}

	if (il != nullptr)
	{
		SCAPES_PROFILER_N("ResourcePipeline<Shader>::loadFromMemory(): create_from_spirv");
		shader->shader = device->createShaderFromIL(
			static_cast<foundation::render::ShaderType>(il->type),
			static_cast<foundation::render::ShaderILType>(il->il_type),
			il->bytecode_size,
			il->bytecode_data
		);
	}

	compiler->releaseShaderIL(il);
	return shader->shader != nullptr;
}
