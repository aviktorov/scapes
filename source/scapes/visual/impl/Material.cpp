#include "Material.h"

#include <scapes/foundation/io/FileSystem.h>

#include <algorithm>

namespace yaml = scapes::foundation::serde::yaml;
namespace math = scapes::foundation::math;

/*
 */
size_t ResourceTraits<scapes::visual::Material>::size()
{
	return sizeof(scapes::visual::impl::Material);
}

void ResourceTraits<scapes::visual::Material>::create(
	scapes::foundation::resources::ResourceManager *resource_manager,
	void *memory,
	scapes::visual::hardware::Device *device,
	scapes::visual::shaders::Compiler *compiler
)
{
	new (memory) scapes::visual::impl::Material(resource_manager, device, compiler);
}

void ResourceTraits<scapes::visual::Material>::destroy(
	scapes::foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	scapes::visual::impl::Material *material = reinterpret_cast<scapes::visual::impl::Material *>(memory);
	material->~Material();
}

scapes::foundation::resources::hash_t ResourceTraits<scapes::visual::Material>::fetchHash(
	scapes::foundation::resources::ResourceManager *resource_manager,
	scapes::foundation::io::FileSystem *file_system,
	void *memory,
	const scapes::foundation::io::URI &uri
)
{
	return 0;
}

bool ResourceTraits<scapes::visual::Material>::reload(
	scapes::foundation::resources::ResourceManager *resource_manager,
	scapes::foundation::io::FileSystem *file_system,
	void *memory,
	const scapes::foundation::io::URI &uri
)
{
	return false;
}

bool ResourceTraits<scapes::visual::Material>::loadFromMemory(
	scapes::foundation::resources::ResourceManager *resource_manager,
	void *memory,
	const uint8_t *data,
	size_t size
)
{
	scapes::visual::impl::Material *material = reinterpret_cast<scapes::visual::impl::Material *>(memory);

	yaml::csubstr yaml(reinterpret_cast<const char *>(data), size);
	yaml::Tree &tree = yaml::parse(yaml);

	return material->deserialize(tree);
}

namespace scapes::visual::impl
{
	/*
	 */
	Material::Material(
		foundation::resources::ResourceManager *resource_manager,
		hardware::Device *device,
		shaders::Compiler *compiler
	)
		: resource_manager(resource_manager), device(device), compiler(compiler), gpu_bindings(resource_manager, device)
	{

	}

	Material::~Material()
	{
	}

	/*
	 */
	MaterialHandle Material::clone()
	{
		MaterialHandle result = resource_manager->create<visual::Material>(device, compiler);
		Material *raw_material = static_cast<Material *>(result.get());

		gpu_bindings.copyTo(raw_material->gpu_bindings);
		return result;
	}

	/*
	 */
	bool Material::flush()
	{
		return gpu_bindings.flush();
	}

	/*
	 */
	bool Material::deserialize(const yaml::Tree &tree)
	{
		// TODO: split this into multiple methods for the sake of readability
		const yaml::NodeRef stream = tree.rootref();

		if (!gpu_bindings.deserialize(stream))
		{
			foundation::Log::error("Material::deserialize(): can't deserialize parameter groups\n");
			return false;
		}

		return true;
	}

	yaml::Tree Material::serialize()
	{
		yaml::Tree tree;
		yaml::NodeRef root = tree.rootref();
		root |= yaml::STREAM;

		gpu_bindings.serialize(root);

		return tree;
	}

	/*
	 */
	bool Material::addGroup(const char *name)
	{
		return gpu_bindings.addGroup(name);
	}

	bool Material::removeGroup(const char *name)
	{
		return gpu_bindings.removeGroup(name);
	}

	void Material::removeAllGroups()
	{
		gpu_bindings.clear();
	}

	hardware::BindSet Material::getGroupBindings(const char *name) const
	{
		return gpu_bindings.getGroupBindings(name);
	}

	/*
	 */
	bool Material::addGroupParameter(const char *group_name, const char *parameter_name, GroupParameterType type, size_t num_elements)
	{
		return gpu_bindings.addGroupParameter(group_name, parameter_name, type, num_elements);
	}

	bool Material::addGroupParameter(const char *group_name, const char *parameter_name, size_t element_size, size_t num_elements)
	{
		return gpu_bindings.addGroupParameter(group_name, parameter_name, element_size, num_elements);
	}

	bool Material::removeGroupParameter(const char *group_name, const char *parameter_name)
	{
		return gpu_bindings.removeGroupParameter(group_name, parameter_name);
	}

	/*
	 */
	bool Material::addGroupTexture(const char *group_name, const char *texture_name)
	{
		return gpu_bindings.addGroupTexture(group_name, texture_name);
	}

	bool Material::removeGroupTexture(const char *group_name, const char *texture_name)
	{
		return gpu_bindings.removeGroupTexture(group_name, texture_name);
	}

	/*
	 */
	TextureHandle Material::getGroupTexture(const char *group_name, const char *texture_name) const
	{
		return gpu_bindings.getGroupTexture(group_name, texture_name);
	}

	bool Material::setGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle)
	{
		return gpu_bindings.setGroupTexture(group_name, texture_name, handle);
	}

	/*
	 */
	size_t Material::getGroupParameterElementSize(const char *group_name, const char *parameter_name) const
	{
		return gpu_bindings.getGroupParameterElementSize(group_name, parameter_name);
	}

	size_t Material::getGroupParameterNumElements(const char *group_name, const char *parameter_name) const
	{
		return gpu_bindings.getGroupParameterNumElements(group_name, parameter_name);
	}

	const void *Material::getGroupParameter(const char *group_name, const char *parameter_name, size_t index) const
	{
		return gpu_bindings.getGroupParameter(group_name, parameter_name, index);
	}

	bool Material::setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_index, size_t num_src_elements, const void *src_data)
	{
		return gpu_bindings.setGroupParameter(group_name, parameter_name, dst_index, num_src_elements, src_data);
	}
}
