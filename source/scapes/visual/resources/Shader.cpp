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
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	resources::Shader *shader = reinterpret_cast<resources::Shader *>(memory);
	assert(shader);

	foundation::shaders::Compiler *compiler = shader->compiler;
	assert(compiler);

	return compiler->getHash(static_cast<foundation::shaders::ShaderType>(shader->type), uri);
}

bool ResourceTraits<resources::Shader>::reload(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	assert(file_system);

	resources::Shader *shader = reinterpret_cast<resources::Shader *>(memory);
	assert(shader);

	resources::Shader temp = {};
	temp.type = shader->type;
	temp.device = shader->device;
	temp.compiler = shader->compiler;

	foundation::io::Stream *stream = file_system->open(uri, "rb");
	if (!stream)
	{
		foundation::Log::error("ResourceTraits<Shader>::reload(): can't open \"%s\" file\n", uri.c_str());
		return false;
	}

	size_t size = static_cast<size_t>(stream->size());

	uint8_t *data = new uint8_t[size];
	stream->read(data, sizeof(uint8_t), size);
	file_system->close(stream);

	bool success = loadFromMemory(resource_manager, &temp, data, size);

	delete[] data;
	if (!success)
		return false;

	destroy(resource_manager, memory);
	*shader = temp;

	foundation::Log::message("ResourceTraits<Shader>::reload(): file \"%s\" reloaded successfully\n", uri.c_str());
	return true;

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
		il = compiler->createShaderIL(static_cast<foundation::shaders::ShaderType>(shader->type), static_cast<uint32_t>(size), reinterpret_cast<const char *>(data), foundation::io::URI());
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
