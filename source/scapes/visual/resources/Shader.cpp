#include <scapes/visual/Shader.h>
#include <scapes/visual/shaders/Compiler.h>

#include <scapes/foundation/profiler/Profiler.h>

using namespace scapes;
using namespace scapes::visual;

/*
 */
size_t ResourceTraits<Shader>::size()
{
	return sizeof(Shader);
}

void ResourceTraits<Shader>::create(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	scapes::visual::hardware::ShaderType type,
	scapes::visual::hardware::Device *device,
	visual::shaders::Compiler *compiler
)
{
	Shader *shader = reinterpret_cast<Shader *>(memory);

	*shader = {};
	shader->type = type;
	shader->device = device;
	shader->compiler = compiler;
}

void ResourceTraits<Shader>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	Shader *shader = reinterpret_cast<Shader *>(memory);
	scapes::visual::hardware::Device *device = shader->device;

	assert(device);

	device->destroyShader(shader->shader);

	*shader = {};
}

foundation::resources::hash_t ResourceTraits<Shader>::fetchHash(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	Shader *shader = reinterpret_cast<Shader *>(memory);
	assert(shader);

	visual::shaders::Compiler *compiler = shader->compiler;
	assert(compiler);

	return compiler->getHash(static_cast<visual::shaders::ShaderType>(shader->type), uri);
}

bool ResourceTraits<Shader>::reload(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	assert(file_system);

	Shader *shader = reinterpret_cast<Shader *>(memory);
	assert(shader);

	Shader temp = {};
	temp.type = shader->type;
	temp.device = shader->device;
	temp.compiler = shader->compiler;

	size_t size = 0;
	uint8_t *data = reinterpret_cast<uint8_t *>(file_system->map(uri, size));

	if (!data)
	{
		foundation::Log::error("ResourceTraits<Shader>::reload(): can't open \"%s\" file\n", uri.c_str());
		return false;
	}

	bool success = loadFromMemory(resource_manager, &temp, data, size);

	file_system->unmap(data);

	if (!success)
		return false;

	destroy(resource_manager, memory);
	*shader = temp;

	foundation::Log::message("ResourceTraits<Shader>::reload(): file \"%s\" reloaded successfully\n", uri.c_str());
	return true;

}

bool ResourceTraits<Shader>::loadFromMemory(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	const uint8_t *data,
	size_t size
)
{
	SCAPES_PROFILER();

	Shader *shader = reinterpret_cast<Shader *>(memory);
	assert(shader);

	scapes::visual::hardware::Device *device = shader->device;
	assert(device);

	visual::shaders::Compiler *compiler = shader->compiler;
	assert(compiler);

	visual::shaders::ShaderIL *il = nullptr;
	
	{
		SCAPES_PROFILER_N("ResourcePipeline<Shader>::loadFromMemory(): compile_to_spirv");
		il = compiler->createShaderIL(static_cast<visual::shaders::ShaderType>(shader->type), static_cast<uint32_t>(size), reinterpret_cast<const char *>(data), foundation::io::URI());
	}

	if (il != nullptr)
	{
		SCAPES_PROFILER_N("ResourcePipeline<Shader>::loadFromMemory(): create_from_spirv");
		shader->shader = device->createShaderFromIL(
			static_cast<scapes::visual::hardware::ShaderType>(il->type),
			static_cast<scapes::visual::hardware::ShaderILType>(il->il_type),
			il->bytecode_size,
			il->bytecode_data
		);
	}

	compiler->releaseShaderIL(il);
	return shader->shader != nullptr;
}
