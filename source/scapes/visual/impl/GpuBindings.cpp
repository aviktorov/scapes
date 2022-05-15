#include "GpuBindings.h"
#include <HashUtils.h>

#include <scapes/foundation/io/FileSystem.h>

namespace scapes::visual::impl
{
	namespace yaml = scapes::foundation::serde::yaml;

	/*
	 */
	static size_t getGroupParameterTypeSize(GroupParameterType type)
	{
		static size_t supported_formats[static_cast<size_t>(GroupParameterType::MAX)] =
		{
			0,
			sizeof(float), sizeof(int32_t), sizeof(uint32_t),
			sizeof(foundation::math::vec2), sizeof(foundation::math::vec3), sizeof(foundation::math::vec4),
			sizeof(foundation::math::ivec2), sizeof(foundation::math::ivec3), sizeof(foundation::math::ivec4),
			sizeof(foundation::math::uvec2), sizeof(foundation::math::uvec3), sizeof(foundation::math::uvec4),
			sizeof(foundation::math::mat3), sizeof(foundation::math::mat4),
		};

		return supported_formats[static_cast<size_t>(type)];
	}

	/*
	 */
	void *ParameterAllocator::allocate(size_t size)
	{
		void *memory = malloc(size);
		memset(memory, 0, size);
		return memory;
	}

	void ParameterAllocator::deallocate(void *memory)
	{
		free(memory);
	}

	/*
	 */
	GpuBindings::GpuBindings(
		foundation::resources::ResourceManager *resource_manager,
		hardware::Device *device
	)
		: resource_manager(resource_manager), device(device)
	{

	}

	GpuBindings::~GpuBindings()
	{
		clear();
	}

	/*
	 */
	void GpuBindings::clear()
	{
		for (auto &[hash, group] : group_lookup)
		{
			clearGroup(group);
			delete group;
		}

		group_lookup.clear();
		group_parameter_lookup.clear();
		group_texture_lookup.clear();
	}

	void GpuBindings::invalidate()
	{

	}

	bool GpuBindings::flush()
	{
		bool should_invalidate = false;

		for (auto &[hash, group] : group_lookup)
		{
			bool result = flushGroup(group);
			should_invalidate = should_invalidate || result;
		}

		return should_invalidate;
	}

	/*
	 */
	bool GpuBindings::deserialize(const yaml::NodeRef root)
	{
		if (!root.is_root())
		{
			foundation::Log::error("GpuBindings::deserialize(): can't get root node\n");
			return false;
		}

		if (!root.is_stream())
		{
			foundation::Log::error("GpuBindings::deserialize(): root node is not a stream\n");
			return false;
		}

		if (root.is_doc())
		{
			foundation::Log::error("GpuBindings::deserialize(): root node must not be a document\n");
			return false;
		}

		for (const yaml::NodeRef document : root.children())
		{
			if (!document.is_doc())
			{
				foundation::Log::error("GpuBindings::deserialize(): child node is not a document\n");
				return false;
			}

			for (const yaml::NodeRef child : document.children())
			{
				yaml::csubstr child_key = child.key();

				if (child_key.compare("ParameterGroup") == 0)
					deserializeGroup(child);
			}
		}

		return true;
	}

	bool GpuBindings::serialize(yaml::NodeRef root)
	{
		if (!root.is_root())
		{
			foundation::Log::error("GpuBindings::serialize(): can't get root node\n");
			return false;
		}

		if (!root.is_stream())
		{
			foundation::Log::error("GpuBindings::serialize(): root node is not a stream\n");
			return false;
		}

		if (root.is_doc())
		{
			foundation::Log::error("GpuBindings::serialize(): root node must not be a document\n");
			return false;
		}

		// serialize all parameter groups
		for (auto &[hash, group] : group_lookup)
		{
			yaml::NodeRef child = root.append_child();
			child |= yaml::DOCMAP;

			child = child.append_child();
			child |= yaml::MAP;

			child.set_key("ParameterGroup");

			child["name"] << group->name.c_str();

			if (group->parameters.size() > 0)
			{
				yaml::NodeRef parameters = child.append_child();
				parameters |= yaml::SEQ;
				parameters.set_key("parameters");

				for (size_t i = 0; i < group->parameters.size(); ++i)
				{
					yaml::NodeRef parameter_node = parameters.append_child();
					parameter_node |= yaml::MAP;

					const GroupParameter *parameter = group->parameters[i];

					parameter_node["name"] << parameter->name.c_str();
					if (parameter->type != GroupParameterType::UNDEFINED)
						parameter_node["type"] << parameter->type;
					else
						parameter_node["size"] << parameter->element_size;

					if (parameter->num_elements > 1)
						parameter_node["elements"] << parameter->num_elements;

					// TODO: serialize current / default value?
				}
			}

			if (group->textures.size() > 0)
			{
				yaml::NodeRef textures = child.append_child();
				textures |= yaml::SEQ;
				textures.set_key("textures");

				for (size_t i = 0; i < group->textures.size(); ++i)
				{
					yaml::NodeRef texture_node = textures.append_child();
					texture_node |= yaml::MAP;

					const GroupTexture *texture = group->textures[i];

					texture_node["name"] << texture->name.c_str();

					const foundation::io::URI &uri = resource_manager->getUri(texture->texture);
					if (!uri.empty())
						texture_node["path"] << uri.c_str();
				}
			}
		}

		return true;
	}

	/*
	 */
	bool GpuBindings::addGroup(const char *name)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		if (group_lookup.find(hash) != group_lookup.end())
			return false;

		Group *group = new Group();
		group->name = std::string(name);

		group_lookup.insert({hash, group});

		return true;
	}

	bool GpuBindings::removeGroup(const char *name)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = group_lookup.find(hash);
		if (it == group_lookup.end())
			return false;

		Group *group = it->second;
		group_lookup.erase(it);

		clearGroup(group);
		delete group;

		return true;
	}

	bool GpuBindings::clearGroup(const char *name)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = group_lookup.find(hash);
		if (it == group_lookup.end())
			return false;

		Group *group = it->second;
		group_lookup.erase(it);

		clearGroup(group);
		return true;
	}

	/*
	 */
	hardware::BindSet GpuBindings::getGroupBindings(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = group_lookup.find(hash);
		if (it == group_lookup.end())
			return SCAPES_NULL_HANDLE;

		const Group *group = it->second;
		return group->bindings;
	}

	/*
	 */
	bool GpuBindings::addGroupParameter(const char *group_name, const char *parameter_name, size_t element_size, size_t num_elements)
	{
		return addGroupParameterInternal(group_name, parameter_name, GroupParameterType::UNDEFINED, element_size, num_elements);
	}

	bool GpuBindings::addGroupParameter(const char *group_name, const char *parameter_name, GroupParameterType type, size_t num_elements)
	{
		size_t element_size = getGroupParameterTypeSize(type);
		return addGroupParameterInternal(group_name, parameter_name, type, element_size, num_elements);
	}

	bool GpuBindings::removeGroupParameter(const char *group_name, const char *parameter_name)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto group_it = group_lookup.find(group_hash);
		if (group_it == group_lookup.end())
			return false;

		uint64_t parameter_hash = group_hash;
		common::HashUtils::combine(parameter_hash, std::string_view(parameter_name));

		auto parameter_it = group_parameter_lookup.find(parameter_hash);
		if (parameter_it == group_parameter_lookup.end())
			return false;

		Group *group = group_it->second;
		GroupParameter *parameter = parameter_it->second;

		group_parameter_lookup.erase(parameter_hash);

		auto it = std::find(group->parameters.begin(), group->parameters.end(), parameter);
		assert(it != group->parameters.end());

		group->parameters.erase(it);

		invalidateGroup(group);

		parameter_allocator.deallocate(parameter->memory);
		delete parameter;

		return true;
	}

	/*
	 */
	bool GpuBindings::addGroupTexture(const char *group_name, const char *texture_name)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto it = group_lookup.find(group_hash);
		if (it == group_lookup.end())
			return false;

		uint64_t texture_hash = group_hash;
		common::HashUtils::combine(texture_hash, std::string_view(texture_name));

		if (group_texture_lookup.find(texture_hash) != group_texture_lookup.end())
			return false;

		Group *group = it->second;

		GroupTexture *texture = new GroupTexture();
		texture->name = std::string(texture_name);
		texture->group = group;

		group->textures.push_back(texture);
		group_texture_lookup.insert({texture_hash, texture});

		invalidateGroup(group);

		return true;
	}

	bool GpuBindings::removeGroupTexture(const char *group_name, const char *texture_name)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto group_it = group_lookup.find(group_hash);
		if (group_it == group_lookup.end())
			return false;

		uint64_t texture_hash = group_hash;
		common::HashUtils::combine(texture_hash, std::string_view(texture_name));

		auto texture_it = group_texture_lookup.find(texture_hash);
		if (texture_it == group_texture_lookup.end())
			return false;

		Group *group = group_it->second;
		GroupTexture *texture = texture_it->second;

		group_texture_lookup.erase(texture_hash);

		auto it = std::find(group->textures.begin(), group->textures.end(), texture);
		assert(it != group->textures.end());

		group->textures.erase(it);

		invalidateGroup(group);
		delete texture;

		return true;
	}

	/*
	 */
	size_t GpuBindings::getGroupParameterElementSize(const char *group_name, const char *parameter_name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(group_name));
		common::HashUtils::combine(hash, std::string_view(parameter_name));

		auto it = group_parameter_lookup.find(hash);
		if (it == group_parameter_lookup.end())
			return 0;

		GroupParameter *parameter = it->second;
		return parameter->element_size;
	}

	size_t GpuBindings::getGroupParameterNumElements(const char *group_name, const char *parameter_name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(group_name));
		common::HashUtils::combine(hash, std::string_view(parameter_name));

		auto it = group_parameter_lookup.find(hash);
		if (it == group_parameter_lookup.end())
			return 0;

		GroupParameter *parameter = it->second;
		return parameter->num_elements;
	}

	const void *GpuBindings::getGroupParameter(const char *group_name, const char *parameter_name, size_t index) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(group_name));
		common::HashUtils::combine(hash, std::string_view(parameter_name));

		auto it = group_parameter_lookup.find(hash);
		if (it == group_parameter_lookup.end())
			return nullptr;

		GroupParameter *parameter = it->second;
		const uint8_t *data = reinterpret_cast<const uint8_t *>(parameter->memory);

		if (index >= parameter->num_elements)
			return nullptr;

		return data + index * parameter->element_size;
	}

	bool GpuBindings::setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_index, size_t num_src_elements, const void *src_data)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto group_it = group_lookup.find(group_hash);
		if (group_it == group_lookup.end())
			return false;

		uint64_t parameter_hash = group_hash;
		common::HashUtils::combine(parameter_hash, std::string_view(parameter_name));

		auto parameter_it = group_parameter_lookup.find(parameter_hash);
		if (parameter_it == group_parameter_lookup.end())
			return false;

		Group *group = group_it->second;
		GroupParameter *parameter = parameter_it->second;
		uint8_t *dst_data = reinterpret_cast<uint8_t *>(parameter->memory);

		if (dst_index >= parameter->num_elements)
			return false;

		if ((parameter->num_elements - dst_index) < num_src_elements)
			return false;

		memcpy(dst_data + dst_index * parameter->element_size, src_data, num_src_elements * parameter->element_size);

		group->dirty = true;

		return true;
	}

	/*
	 */
	TextureHandle GpuBindings::getGroupTexture(const char *group_name, const char *texture_name) const
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto group_it = group_lookup.find(group_hash);
		if (group_it == group_lookup.end())
			return TextureHandle();

		uint64_t texture_hash = group_hash;
		common::HashUtils::combine(texture_hash, std::string_view(texture_name));

		auto texture_it = group_texture_lookup.find(texture_hash);
		if (texture_it == group_texture_lookup.end())
			return TextureHandle();

		GroupTexture *texture = texture_it->second;

		return texture->texture;
	}

	bool GpuBindings::setGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto group_it = group_lookup.find(group_hash);
		if (group_it == group_lookup.end())
			return false;

		uint64_t texture_hash = group_hash;
		common::HashUtils::combine(texture_hash, std::string_view(texture_name));

		auto texture_it = group_texture_lookup.find(texture_hash);
		if (texture_it == group_texture_lookup.end())
			return false;

		GroupTexture *texture = texture_it->second;

		texture->texture = handle;
		return true;
	}

	/*
	 */
	bool GpuBindings::addGroupParameterInternal(const char *group_name, const char *parameter_name, GroupParameterType type, size_t element_size, size_t num_elements)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto it = group_lookup.find(group_hash);
		if (it == group_lookup.end())
			return false;

		uint64_t parameter_hash = group_hash;
		common::HashUtils::combine(parameter_hash, std::string_view(parameter_name));

		if (group_parameter_lookup.find(parameter_hash) != group_parameter_lookup.end())
			return false;

		Group *group = it->second;

		GroupParameter *parameter = new GroupParameter();
		parameter->name = std::string(parameter_name);
		parameter->group = group;
		parameter->type = type;
		parameter->element_size = element_size;
		parameter->num_elements = num_elements;
		parameter->memory = parameter_allocator.allocate(parameter->element_size * parameter->num_elements);

		group->parameters.push_back(parameter);
		group_parameter_lookup.insert({parameter_hash, parameter});

		invalidateGroup(group);

		return true;
	}

	/*
	 */
	void GpuBindings::clearGroup(Group *group)
	{
		assert(group);

		invalidateGroup(group);

		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group->name));

		for (GroupParameter *parameter : group->parameters)
		{
			uint64_t parameter_hash = group_hash;
			common::HashUtils::combine(parameter_hash, std::string_view(parameter->name));

			assert(group_parameter_lookup.find(parameter_hash) != group_parameter_lookup.end());
			group_parameter_lookup.erase(parameter_hash);

			parameter_allocator.deallocate(parameter->memory);
			delete parameter;
		}

		for (GroupTexture *texture : group->textures)
		{
			uint64_t texture_hash = group_hash;
			common::HashUtils::combine(texture_hash, std::string_view(texture->name));

			assert(group_texture_lookup.find(texture_hash) != group_texture_lookup.end());
			group_texture_lookup.erase(texture_hash);

			delete texture;
		}

		group->parameters.clear();
		group->textures.clear();
	}

	void GpuBindings::invalidateGroup(Group *group)
	{
		assert(group);

		// TODO: make sure UBO + BindSet recreated only if
		// current UBO size is not enough to fit all parameters
		device->destroyUniformBuffer(group->buffer);
		device->destroyBindSet(group->bindings);

		group->buffer = nullptr;
		group->buffer_size = 0;
		group->bindings = nullptr;

		group->dirty = true;
	}

	bool GpuBindings::flushGroup(Group *group)
	{
		assert(group);

		if (!group->dirty)
			return false;

		bool should_invalidate = false;

		uint32_t current_offset = 0;
		uint32_t ubo_size = 0;
		constexpr uint32_t alignment = 16;

		for (GroupParameter *parameter : group->parameters)
		{
			uint32_t padding = alignment - current_offset % alignment;
			uint32_t total_size = static_cast<uint32_t>(parameter->element_size * parameter->num_elements);

			if (current_offset > 0 && current_offset + total_size > alignment)
				ubo_size += padding;

			ubo_size += static_cast<uint32_t>(total_size);
			current_offset = (current_offset + total_size) % alignment;
		}

		if (group->buffer_size < ubo_size)
		{
			device->destroyUniformBuffer(group->buffer);
			device->destroyBindSet(group->bindings);

			group->buffer_size = ubo_size;
			group->buffer = device->createUniformBuffer(hardware::BufferType::DYNAMIC, ubo_size);

			should_invalidate = true;
		}

		if (group->buffer_size > 0)
		{
			uint8_t *ubo_data = reinterpret_cast<uint8_t *>(device->map(group->buffer));

			current_offset = 0;
			for (GroupParameter *parameter : group->parameters)
			{
				size_t padding = alignment - current_offset % alignment;
				uint32_t total_size = static_cast<uint32_t>(parameter->element_size * parameter->num_elements);

				if (current_offset > 0 && current_offset + total_size > alignment)
					ubo_data += padding;

				memcpy(ubo_data, parameter->memory, total_size);

				ubo_data += total_size;
				current_offset = (current_offset + total_size) % alignment;
			}

			device->unmap(group->buffer);
		}

		if (group->bindings == nullptr)
		{
			group->bindings = device->createBindSet();
			should_invalidate = true;
		}

		uint32_t binding = 0;
		if (group->buffer)
			device->bindUniformBuffer(group->bindings, binding++, group->buffer);

		for (GroupTexture *group_texture : group->textures)
		{
			TextureHandle handle = group_texture->texture;
			Texture *texture = handle.get();

			device->bindTexture(group->bindings, binding++, (texture) ? texture->gpu_data : nullptr);
		}

		group->dirty = false;
		return should_invalidate;
	}

	/*
	 */
	void GpuBindings::deserializeGroup(yaml::NodeRef group_node)
	{
		std::string group_name;
		std::vector<yaml::NodeRef> parameters;
		std::vector<yaml::NodeRef> textures;

		for (const yaml::NodeRef group_child : group_node.children())
		{
			yaml::csubstr group_child_key = group_child.key();

			if (group_child_key.compare("name") == 0 && group_child.has_val())
			{
				yaml::csubstr group_child_value = group_child.val();
				group_name = std::string(group_child_value.data(), group_child_value.size());
			}
			else if (group_child_key.compare("parameters") == 0)
			{
				const yaml::NodeRef parameter_data = group_child.first_child();
				for (const yaml::NodeRef parameter_container : parameter_data.siblings())
					parameters.push_back(parameter_container);
			}
			else if (group_child_key.compare("textures") == 0)
			{
				const yaml::NodeRef texture_data = group_child.first_child();
				for (const yaml::NodeRef texture_container : texture_data.siblings())
					textures.push_back(texture_container);
			}
		}

		if (group_name.empty())
			return;

		addGroup(group_name.c_str());

		for (const yaml::NodeRef parameter : parameters)
			deserializeGroupParameter(group_name.c_str(), parameter);

		for (const yaml::NodeRef texture : textures)
			deserializeGroupTexture(group_name.c_str(), texture);

	}

	void GpuBindings::deserializeGroupParameter(const char *group_name, yaml::NodeRef parameter_node)
	{
		std::string parameter_name;
		size_t parameter_num_elements = 1;
		size_t parameter_element_size = 0;
		GroupParameterType parameter_type = GroupParameterType::UNDEFINED;
		yaml::NodeRef parameter_value;

		for (const yaml::NodeRef parameter_child : parameter_node.children())
		{
			yaml::csubstr parameter_child_key = parameter_child.key();

			if (parameter_child_key.compare("name") == 0 && parameter_child.has_val())
			{
				yaml::csubstr parameter_child_value = parameter_child.val();
				parameter_name = std::string(parameter_child_value.data(), parameter_child_value.size());
			}
			else if (parameter_child_key.compare("type") == 0 && parameter_child.has_val())
				parameter_child >> parameter_type;
			// TODO: check if we already remembered the value
			else if (parameter_child_key.compare("value") == 0)
				parameter_value = parameter_child;
			else if (parameter_child_key.compare("size") == 0 && parameter_child.has_val())
				parameter_child >> parameter_element_size;
			else if (parameter_child_key.compare("elements") == 0 && parameter_child.has_val())
				parameter_child >> parameter_num_elements;
		}

		if (parameter_type != GroupParameterType::UNDEFINED)
			parameter_element_size = getGroupParameterTypeSize(parameter_type);

		if (parameter_name.empty() || parameter_element_size * parameter_num_elements == 0)
			return;

		if (parameter_type != GroupParameterType::UNDEFINED)
			addGroupParameter(group_name, parameter_name.c_str(), parameter_type, parameter_num_elements);
		else
			addGroupParameter(group_name, parameter_name.c_str(), parameter_element_size, parameter_num_elements);

		if (!parameter_value.valid())
			return;

		auto parseValue = [this, group_name, parameter_name, parameter_type](const yaml::NodeRef node, size_t index)
		{
			union GroupParameterValue
			{
				float f;
				int32_t i;
				uint32_t u;
				foundation::math::vec2 v2;
				foundation::math::vec3 v3;
				foundation::math::vec4 v4;
				foundation::math::ivec2 i2;
				foundation::math::ivec3 i3;
				foundation::math::ivec4 i4;
				foundation::math::uvec2 u2;
				foundation::math::uvec3 u3;
				foundation::math::uvec4 u4;
				foundation::math::mat3 m3;
				foundation::math::mat4 m4;
			};

			GroupParameterValue value;
			switch (parameter_type)
			{
				case GroupParameterType::FLOAT: node >> value.f; break;
				case GroupParameterType::INT: node >> value.i; break;
				case GroupParameterType::UINT: node >> value.u; break;
				case GroupParameterType::VEC2: node >> value.v2; break;
				case GroupParameterType::VEC3: node >> value.v3; break;
				case GroupParameterType::VEC4: node >> value.v4; break;
				case GroupParameterType::IVEC2: node >> value.i2; break;
				case GroupParameterType::IVEC3: node >> value.i3; break;
				case GroupParameterType::IVEC4: node >> value.i4; break;
				case GroupParameterType::UVEC2: node >> value.u2; break;
				case GroupParameterType::UVEC3: node >> value.u3; break;
				case GroupParameterType::UVEC4: node >> value.u4; break;
				case GroupParameterType::MAT3: node >> value.m3; break;
				case GroupParameterType::MAT4: node >> value.m4; break;
			}

			setGroupParameter(group_name, parameter_name.c_str(), index, 1, &value);
		};

		if (parameter_num_elements == 1)
		{
			parseValue(parameter_value, 0);
		}
		else
		{
			const yaml::NodeRef value_array = parameter_value.first_child();
			size_t dst_index = 0;

			for (const yaml::NodeRef value_child : value_array.siblings())
			{
				parseValue(value_child, dst_index);
				dst_index++;
			}
		}
	}

	void GpuBindings::deserializeGroupTexture(const char *group_name, yaml::NodeRef texture_node)
	{
		std::string texture_name;
		std::string texture_path;

		for (const yaml::NodeRef texture_child : texture_node.children())
		{
			yaml::csubstr texture_child_key = texture_child.key();

			if (texture_child_key.compare("name") == 0 && texture_child.has_val())
			{
				yaml::csubstr texture_child_value = texture_child.val();
				texture_name = std::string(texture_child_value.data(), texture_child_value.size());
			}
			else if (texture_child_key.compare("path") == 0 && texture_child.has_val())
			{
				yaml::csubstr texture_child_value = texture_child.val();
				texture_path = std::string(texture_child_value.data(), texture_child_value.size());
			}
		}

		if (texture_name.empty())
			return;

		addGroupTexture(group_name, texture_name.c_str());

		if (texture_path.empty())
			return;

		TextureHandle handle = resource_manager->fetch<Texture>(texture_path.c_str(), device);
		setGroupTexture(group_name, texture_name.c_str(), handle);
	}
}
