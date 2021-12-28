#include "RenderGraph.h"
#include "HashUtils.h"

#include <scapes/foundation/io/FileSystem.h>

#include <algorithm>

enum class GroupParameterType
{
	UNDEFINED = 0,
	FLOAT,
	INT,
	UINT,
	VEC2,
	VEC3,
	VEC4,
	IVEC2,
	IVEC3,
	IVEC4,
	UVEC2,
	UVEC3,
	UVEC4,
	MAT3,
	MAT4,

	MAX,
};

union GroupParameterValue
{
	float f;
	int32_t i;
	uint32_t u;
	scapes::foundation::math::vec2 v2;
	scapes::foundation::math::vec3 v3;
	scapes::foundation::math::vec4 v4;
	scapes::foundation::math::ivec2 i2;
	scapes::foundation::math::ivec3 i3;
	scapes::foundation::math::ivec4 i4;
	scapes::foundation::math::uvec2 u2;
	scapes::foundation::math::uvec3 u3;
	scapes::foundation::math::uvec4 u4;
	scapes::foundation::math::mat3 m3;
	scapes::foundation::math::mat4 m4;
};

static GroupParameterValue parseGroupParameterValue(GroupParameterType type, const scapes::foundation::serde::yaml::NodeRef node)
{
	GroupParameterValue ret;

	switch (type)
	{
		case GroupParameterType::FLOAT: node >> ret.f; break;
		case GroupParameterType::INT: node >> ret.i; break;
		case GroupParameterType::UINT: node >> ret.u; break;
		case GroupParameterType::VEC2: node >> ret.v2; break;
		case GroupParameterType::VEC3: node >> ret.v3; break;
		case GroupParameterType::VEC4: node >> ret.v4; break;
		case GroupParameterType::IVEC2: node >> ret.i2; break;
		case GroupParameterType::IVEC3: node >> ret.i3; break;
		case GroupParameterType::IVEC4: node >> ret.i4; break;
		case GroupParameterType::UVEC2: node >> ret.u2; break;
		case GroupParameterType::UVEC3: node >> ret.u3; break;
		case GroupParameterType::UVEC4: node >> ret.u4; break;
		case GroupParameterType::MAT3: node >> ret.m3; break;
		case GroupParameterType::MAT4: node >> ret.m4; break;
	}

	return ret;
}

static size_t getGroupParameterTypeSize(GroupParameterType type)
{
	static size_t supported_formats[GroupParameterType::MAX] =
	{
		0,
		sizeof(float), sizeof(int32_t), sizeof(uint32_t),
		sizeof(scapes::foundation::math::vec2), sizeof(scapes::foundation::math::vec3), sizeof(scapes::foundation::math::vec4),
		sizeof(scapes::foundation::math::ivec2), sizeof(scapes::foundation::math::ivec3), sizeof(scapes::foundation::math::ivec4),
		sizeof(scapes::foundation::math::uvec2), sizeof(scapes::foundation::math::uvec3), sizeof(scapes::foundation::math::uvec4),
		sizeof(scapes::foundation::math::mat3), sizeof(scapes::foundation::math::mat4),
	};

	return supported_formats[static_cast<size_t>(type)];
}

namespace c4
{
	/*
	 */
	SCAPES_INLINE bool from_chars(const scapes::foundation::serde::yaml::csubstr buf, scapes::foundation::render::Format *format)
	{
		static const char *supported_formats[scapes::foundation::render::Format::MAX] =
		{
			"UNDEFINED",

			"R8_UNORM", "R8_SNORM", "R8_UINT", "R8_SINT",
			"R8G8_UNORM", "R8G8_SNORM", "R8G8_UINT", "R8G8_SINT",
			"R8G8B8_UNORM", "R8G8B8_SNORM", "R8G8B8_UINT", "R8G8B8_SINT",
			"B8G8R8_UNORM", "B8G8R8_SNORM", "B8G8R8_UINT", "B8G8R8_SINT",
			"R8G8B8A8_UNORM", "R8G8B8A8_SNORM", "R8G8B8A8_UINT", "R8G8B8A8_SINT",
			"B8G8R8A8_UNORM", "B8G8R8A8_SNORM", "B8G8R8A8_UINT", "B8G8R8A8_SINT",

			"R16_UNORM", "R16_SNORM", "R16_UINT", "R16_SINT", "R16_SFLOAT", "R16G16_UNORM",
			"R16G16_SNORM", "R16G16_UINT", "R16G16_SINT", "R16G16_SFLOAT",
			"R16G16B16_UNORM", "R16G16B16_SNORM", "R16G16B16_UINT", "R16G16B16_SINT", "R16G16B16_SFLOAT",
			"R16G16B16A16_UNORM", "R16G16B16A16_SNORM", "R16G16B16A16_UINT", "R16G16B16A16_SINT", "R16G16B16A16_SFLOAT",

			"R32_UINT", "R32_SINT", "R32_SFLOAT",
			"R32G32_UINT", "R32G32_SINT", "R32G32_SFLOAT",
			"R32G32B32_UINT", "R32G32B32_SINT", "R32G32B32_SFLOAT",
			"R32G32B32A32_UINT", "R32G32B32A32_SINT", "R32G32B32A32_SFLOAT",

			"D16_UNORM", "D16_UNORM_S8_UINT", "D24_UNORM", "D24_UNORM_S8_UINT", "D32_SFLOAT", "D32_SFLOAT_S8_UINT",
		};

		for (size_t i = 0; i < static_cast<size_t>(scapes::foundation::render::Format::MAX); ++i)
		{
			if (buf.compare(supported_formats[i], strlen(supported_formats[i])) == 0)
			{
				*format = static_cast<scapes::foundation::render::Format>(i);
				return true;
			}
		}

		return false;
	}

	/*
	 */
	using qualifier = scapes::foundation::math::qualifier;

	template<typename T, qualifier Q> bool from_chars(const scapes::foundation::serde::yaml::csubstr buf, scapes::foundation::math::vec<2, T, Q> *vec)
	{
		size_t ret = scapes::foundation::serde::yaml::unformat(buf, "{},{}", vec->x, vec->y);
		return ret != ryml::yml::npos;
	}

	template<typename T, qualifier Q> bool from_chars(const scapes::foundation::serde::yaml::csubstr buf, scapes::foundation::math::vec<3, T, Q> *vec)
	{
		size_t ret = scapes::foundation::serde::yaml::unformat(buf, "{},{},{}", vec->x, vec->y, vec->z);
		return ret != ryml::yml::npos;
	}

	template<typename T, qualifier Q> bool from_chars(const scapes::foundation::serde::yaml::csubstr buf, scapes::foundation::math::vec<4, T, Q> *vec)
	{
		size_t ret = scapes::foundation::serde::yaml::unformat(buf, "{},{},{},{}", vec->x, vec->y, vec->z, vec->w);
		return ret != ryml::yml::npos;
	}

	SCAPES_INLINE bool from_chars(const scapes::foundation::serde::yaml::csubstr buf, scapes::foundation::math::mat3 *mat)
	{
		scapes::foundation::math::vec3 &c0 = (*mat)[0];
		scapes::foundation::math::vec3 &c1 = (*mat)[1];
		scapes::foundation::math::vec3 &c2 = (*mat)[2];

		size_t ret = scapes::foundation::serde::yaml::unformat(
			buf,
			"{},{},{},{},{},{},{},{},{}",
			c0.x, c0.y, c0.z,
			c1.x, c1.y, c1.z,
			c2.x, c2.y, c2.z
		);

		return ret != ryml::yml::npos;
	}

	SCAPES_INLINE bool from_chars(const scapes::foundation::serde::yaml::csubstr buf, scapes::foundation::math::mat4 *mat)
	{
		scapes::foundation::math::vec4 &c0 = (*mat)[0];
		scapes::foundation::math::vec4 &c1 = (*mat)[1];
		scapes::foundation::math::vec4 &c2 = (*mat)[2];
		scapes::foundation::math::vec4 &c3 = (*mat)[3];

		size_t ret = scapes::foundation::serde::yaml::unformat(
			buf,
			"{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}",
			c0.x, c0.y, c0.z, c0.w,
			c1.x, c1.y, c1.z, c1.w,
			c2.x, c2.y, c2.z, c2.w,
			c3.x, c3.y, c3.z, c3.w
		);

		return ret != ryml::yml::npos;
	}

	/*
	 */
	bool from_chars(const scapes::foundation::serde::yaml::csubstr buf, GroupParameterType *type)
	{
		static const char *supported_formats[GroupParameterType::MAX] =
		{
			"undefined",
			"float", "int", "uint",
			"vec2", "vec3", "vec4",
			"ivec2", "ivec3", "ivec4",
			"uvec2", "uvec3", "uvec4",
			"mat3", "mat4",
		};

		// NOTE: start index is intentionally set to 1 in order to skip check for UNDEFINED type
		for (size_t i = 1; i < static_cast<size_t>(GroupParameterType::MAX); ++i)
		{
			if (buf.compare(supported_formats[i], strlen(supported_formats[i])) == 0)
			{
				*type = static_cast<GroupParameterType>(i);
				return true;
			}
		}

		return false;
	}
}

namespace scapes::visual
{
	/*
	 */
	RenderGraph *RenderGraph::create(foundation::render::Device *device, foundation::game::World *world, foundation::io::FileSystem *file_system, foundation::resources::ResourceManager *resource_manager)
	{
		return new RenderGraphImpl(device, world, file_system, resource_manager);
	}

	void RenderGraph::destroy(RenderGraph *RenderGraph)
	{
		delete RenderGraph;
	}

	/*
	 */
	void *ParameterAllocator::allocate(size_t size)
	{
		return malloc(size);
	}

	void ParameterAllocator::deallocate(void *memory)
	{
		free(memory);
	}

	/*
	 */
	RenderGraphImpl::RenderGraphImpl(foundation::render::Device *device, foundation::game::World *world, foundation::io::FileSystem *file_system, foundation::resources::ResourceManager *resource_manager)
		: device(device), world(world), file_system(file_system), resource_manager(resource_manager)
	{

	}

	RenderGraphImpl::~RenderGraphImpl()
	{
		shutdown();

		removeAllGroups();
		removeAllGroupParameters();
		removeAllGroupTextures();
		removeAllRenderBuffers();
		removeAllRenderPasses();

		render_pass_type_lookup.clear();
	}

	/*
	 */
	void RenderGraphImpl::init(uint32_t w, uint32_t h)
	{
		width = w;
		height = h;

		bool should_invalidate = false;

		for (auto &[hash, group] : group_lookup)
		{
			bool result = flushGroup(group);
			should_invalidate = should_invalidate || result;
		}

		for (auto &[hash, render_buffer] : render_buffer_lookup)
		{
			bool result = flushRenderBuffer(render_buffer);
			should_invalidate = should_invalidate || result;
		}

		for (IRenderPass *pass : passes)
			pass->init();

		if (should_invalidate)
			for (IRenderPass *pass : passes)
				pass->invalidate();

	}

	void RenderGraphImpl::shutdown()
	{
		for (IRenderPass *pass : passes)
			pass->shutdown();

		for (auto &[hash, group] : group_lookup)
			invalidateGroup(group);

		for (auto &[hash, render_buffer] : render_buffer_lookup)
			invalidateRenderBuffer(render_buffer);

		invalidateFrameBufferCache();
	}

	/*
	 */
	void RenderGraphImpl::resize(uint32_t w, uint32_t h)
	{
		width = w;
		height = h;

		for (auto &[hash, render_buffer] : render_buffer_lookup)
		{
			invalidateRenderBuffer(render_buffer);
			flushRenderBuffer(render_buffer);
		}

		invalidateFrameBufferCache();

		for (IRenderPass *pass : passes)
			pass->invalidate();
	}

	void RenderGraphImpl::render(foundation::render::CommandBuffer *command_buffer)
	{
		bool should_invalidate = false;

		for (auto &[hash, group] : group_lookup)
		{
			bool result = flushGroup(group);
			should_invalidate = should_invalidate || result;
		}

		for (auto &[hash, render_buffer] : render_buffer_lookup)
		{
			bool result = flushRenderBuffer(render_buffer);
			should_invalidate = should_invalidate || result;
		}

		if (should_invalidate)
		{
			scapes::foundation::Log::message("Invalidation before render\n");
			for (IRenderPass *pass : passes)
				pass->invalidate();
		}

		for (IRenderPass *pass : passes)
			pass->render(command_buffer);
	}

	/*
	 */
	bool RenderGraphImpl::load(const foundation::io::URI &uri)
	{
		foundation::io::Stream *file = file_system->open(uri, "rb");
		if (!file)
		{
			foundation::Log::error("RenderGraph::load(): can't open \"%s\" file\n", uri);
			return false;
		}

		file->seek(0, foundation::io::SeekOrigin::END);
		size_t size = file->tell();
		file->seek(0, foundation::io::SeekOrigin::SET);

		uint8_t *data = new uint8_t[size];
		file->read(data, sizeof(uint8_t), size);
		file_system->close(file);

		foundation::serde::yaml::csubstr yaml(reinterpret_cast<const char *>(data), size);
		foundation::serde::yaml::Tree &tree = foundation::serde::yaml::parse(yaml);
		bool result = deserialize(tree);
		delete[] data;

		return result;
	}

	bool RenderGraphImpl::save(const foundation::io::URI &uri)
	{
		// TODO: implement
		return false;
	}

	/*
	 */
	bool RenderGraphImpl::deserialize(const foundation::serde::yaml::Tree &tree)
	{
		// TODO: split this into multiple methods for the sake of readability
		using namespace foundation::serde;

		const yaml::NodeRef stream = tree.rootref();
		if (!stream.is_root())
		{
			foundation::Log::error("RenderGraph::deserialize(): can't get root node\n");
			return false;
		}

		if (!stream.is_stream())
		{
			foundation::Log::error("RenderGraph::deserialize(): root node is not a stream\n");
			return false;
		}

		if (stream.is_doc())
		{
			foundation::Log::error("RenderGraph::deserialize(): root node must not be a document\n");
			return false;
		}

		for (const yaml::NodeRef document : stream.children())
		{
			if (!document.is_doc())
			{
				foundation::Log::error("RenderGraph::deserialize(): child node is not a document\n");
				return false;
			}

			for (const yaml::NodeRef child : document.children())
			{
				yaml::csubstr child_key = child.key();

				if (child_key.compare("ParameterGroup") == 0)
				{
					std::string group_name;
					std::vector<yaml::NodeRef> parameters;
					std::vector<yaml::NodeRef> textures;

					for (const yaml::NodeRef group_child : child.children())
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

					if (!group_name.empty())
					{
						addGroup(group_name.c_str());

						for (const yaml::NodeRef parameter : parameters)
						{
							std::string parameter_name;
							size_t parameter_count = 1;
							GroupParameterType parameter_type = GroupParameterType::UNDEFINED;
							yaml::NodeRef parameter_value;

							for (const yaml::NodeRef parameter_child : parameter.children())
							{
								yaml::csubstr parameter_child_key = parameter_child.key();

								if (parameter_child_key.compare("name") == 0 && parameter_child.has_val())
								{
									yaml::csubstr parameter_child_value = parameter_child.val();
									parameter_name = std::string(parameter_child_value.data(), parameter_child_value.size());
								}
								else if (parameter_child_key.compare("type") == 0)
									parameter_child >> parameter_type;
								// TODO: check if we already remembered the value
								else if (parameter_child_key.compare("value") == 0)
									parameter_value = parameter_child;
								else if (parameter_child_key.compare("count") == 0)
									parameter_child >> parameter_count;
							}

							size_t parameter_type_size = getGroupParameterTypeSize(parameter_type);
							size_t parameter_size = parameter_type_size * parameter_count;

							if (!parameter_name.empty() && parameter_size > 0)
							{
								addGroupParameter(group_name.c_str(), parameter_name.c_str(), parameter_size);

								if (parameter_value.valid())
								{
									if (parameter_count == 1)
									{
										GroupParameterValue value = parseGroupParameterValue(parameter_type, parameter_value);
										setGroupParameter(group_name.c_str(), parameter_name.c_str(), 0, parameter_type_size, &value);
									}
									else
									{
										const yaml::NodeRef value_array = parameter_value.first_child();
										size_t offset = 0;

										for (const yaml::NodeRef value_child : value_array.siblings())
										{
											GroupParameterValue value = parseGroupParameterValue(parameter_type, value_child);
											setGroupParameter(group_name.c_str(), parameter_name.c_str(), offset, parameter_type_size, &value);
											offset += parameter_type_size;
										}
									}
								}
							}
						}

						for (const yaml::NodeRef texture : textures)
						{
							std::string texture_name;
							std::string texture_path;

							for (const yaml::NodeRef texture_child : texture.children())
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

							if (!texture_name.empty())
							{
								addGroupTexture(group_name.c_str(), texture_name.c_str());

								if (!texture_path.empty())
								{
									// TODO: replace by visual API loadTexture call
									TextureHandle handle = resource_manager->import<resources::Texture>(texture_path.c_str(), device);
									setGroupTexture(group_name.c_str(), texture_name.c_str(), handle);
								}
							}
						}
					}
				}

				if (child_key.compare("RenderBuffers") == 0)
				{
					const yaml::NodeRef renderbuffer_data = child.first_child();

					for (const yaml::NodeRef renderbuffer_container : renderbuffer_data.siblings())
					{
						std::string name;
						foundation::render::Format format = foundation::render::Format::UNDEFINED;
						uint32_t downscale = 1;

						for (const yaml::NodeRef renderbuffer_child : renderbuffer_container.children())
						{
							yaml::csubstr renderbuffer_child_key = renderbuffer_child.key();

							if (renderbuffer_child_key.compare("name") == 0)
							{
								yaml::csubstr renderbuffer_child_value = renderbuffer_child.val();
								name = std::string(renderbuffer_child_value.data(), renderbuffer_child_value.size());
							}
							else if (renderbuffer_child_key.compare("format") == 0)
								renderbuffer_child >> format;
						}

						if (!name.empty() && format != foundation::render::Format::UNDEFINED)
							addRenderBuffer(name.c_str(), format, downscale);
					}
				}

				if (child_key.compare("RenderPass") == 0)
					foundation::Log::message("Found RenderPass\n");
			}
		}

		return true;
	}

	foundation::serde::yaml::Tree RenderGraphImpl::serialize()
	{
		// TODO: implement
		return foundation::serde::yaml::Tree();
	}

	/*
	 */
	bool RenderGraphImpl::addGroup(const char *name)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		if (group_lookup.find(hash) != group_lookup.end())
			return false;

		Group *group = new Group();
		group_lookup.insert({hash, group});

		return true;
	}

	bool RenderGraphImpl::removeGroup(const char *name)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = group_lookup.find(hash);
		if (it == group_lookup.end())
			return false;

		Group *group = it->second;
		group_lookup.erase(it);

		destroyGroup(group);

		return true;
	}

	void RenderGraphImpl::removeAllGroups()
	{
		for (auto &[hash, group] : group_lookup)
			destroyGroup(group);

		group_lookup.clear();
	}

	foundation::render::BindSet *RenderGraphImpl::getGroupBindings(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = group_lookup.find(hash);
		if (it == group_lookup.end())
			return false;

		const Group *group = it->second;
		return group->bindings;
	}

	/*
	 */
	bool RenderGraphImpl::addGroupParameter(const char *group_name, const char *parameter_name, size_t size)
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
		parameter->group = group;
		parameter->size = size;
		parameter->memory = parameter_allocator.allocate(size);

		group->parameters.push_back(parameter);
		group_parameter_lookup.insert({parameter_hash, parameter});

		invalidateGroup(group);

		return true;
	}

	bool RenderGraphImpl::removeGroupParameter(const char *group_name, const char *parameter_name)
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

		destroyGroupParameter(parameter);
		invalidateGroup(group);

		return true;
	}

	void RenderGraphImpl::removeAllGroupParameters()
	{
		for (auto &[hash, parameter] : group_parameter_lookup)
			destroyGroupParameter(parameter);

		group_parameter_lookup.clear();

		for (auto &[hash, group] : group_lookup)
		{
			group->parameters.clear();
			invalidateGroup(group);
		}
	}

	/*
	 */
	bool RenderGraphImpl::addGroupTexture(const char *group_name, const char *texture_name)
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
		texture->group = group;

		group->textures.push_back(texture);
		group_texture_lookup.insert({texture_hash, texture});

		invalidateGroup(group);

		return true;
	}

	bool RenderGraphImpl::removeGroupTexture(const char *group_name, const char *texture_name)
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

		destroyGroupTexture(texture);
		invalidateGroup(group);

		return true;
	}

	void RenderGraphImpl::removeAllGroupTextures()
	{
		for (auto &[hash, texture] : group_texture_lookup)
			destroyGroupTexture(texture);

		group_texture_lookup.clear();

		for (auto &[hash, group] : group_lookup)
		{
			group->textures.clear();
			invalidateGroup(group);
		}
	}

	/*
	 */
	TextureHandle RenderGraphImpl::getGroupTexture(const char *group_name, const char *texture_name) const
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

	bool RenderGraphImpl::setGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle)
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
	bool RenderGraphImpl::addRenderBuffer(const char *name, foundation::render::Format format, uint32_t downscale)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		if (render_buffer_lookup.find(hash) != render_buffer_lookup.end())
			return false;

		RenderBuffer *render_buffer = new RenderBuffer();

		render_buffer->format = format;
		render_buffer->downscale = std::max<uint32_t>(1, downscale);
		render_buffer->texture = nullptr;
		render_buffer->bindings = nullptr;

		render_buffer_lookup.insert({hash, render_buffer});
		return true;
	}

	bool RenderGraphImpl::removeRenderBuffer(const char *name)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return false;

		RenderBuffer *render_buffer = it->second;
		destroyRenderBuffer(render_buffer);

		render_buffer_lookup.erase(hash);
		return true;
	}

	void RenderGraphImpl::removeAllRenderBuffers()
	{
		for (auto &[hash, render_buffer] : render_buffer_lookup)
			destroyRenderBuffer(render_buffer);

		render_buffer_lookup.clear();
	}

	bool RenderGraphImpl::swapRenderBuffers(const char *name0, const char *name1)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name0));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return false;

		RenderBuffer *render_buffer0 = it->second;

		hash = 0;
		common::HashUtils::combine(hash, std::string_view(name1));

		it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return false;

		RenderBuffer *render_buffer1 = it->second;

		if (render_buffer0->format != render_buffer1->format)
			return false;

		if (render_buffer0->downscale != render_buffer1->downscale)
			return false;

		std::swap(render_buffer0->texture, render_buffer1->texture);
		std::swap(render_buffer0->bindings, render_buffer1->bindings);

		return true;
	}

	/*
	 */
	foundation::render::Texture *RenderGraphImpl::getRenderBufferTexture(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return nullptr;

		RenderBuffer *render_buffer = it->second;
		return render_buffer->texture;
	}

	foundation::render::BindSet *RenderGraphImpl::getRenderBufferBindings(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return nullptr;

		RenderBuffer *render_buffer = it->second;
		return render_buffer->bindings;
	}

	foundation::render::Format RenderGraphImpl::getRenderBufferFormat(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return foundation::render::Format::UNDEFINED;

		RenderBuffer *render_buffer = it->second;
		return render_buffer->format;
	}

	uint32_t RenderGraphImpl::getRenderBufferDownscale(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return 0;

		RenderBuffer *render_buffer = it->second;
		return render_buffer->downscale;
	}

	/*
	 */
	foundation::render::FrameBuffer *RenderGraphImpl::fetchFrameBuffer(uint32_t num_attachments, const char *render_buffer_names[])
	{
		uint64_t hash = 0;

		foundation::render::FrameBufferAttachment attachments[32];

		for (uint32_t i = 0; i < num_attachments; ++i)
		{
			foundation::render::FrameBufferAttachment &attachment = attachments[i];
			attachment.texture = getRenderBufferTexture(render_buffer_names[i]);
			attachment.base_layer = 0;
			attachment.base_mip = 0;
			attachment.num_layers = 1;

			common::HashUtils::combine(hash, attachment.texture);
			common::HashUtils::combine(hash, attachment.base_layer);
			common::HashUtils::combine(hash, attachment.base_mip);
			common::HashUtils::combine(hash, attachment.num_layers);
		}

		auto it = framebuffer_cache.find(hash);
		if (it != framebuffer_cache.end())
			return it->second;

		foundation::render::FrameBuffer *framebuffer = device->createFrameBuffer(num_attachments, attachments);
		framebuffer_cache.insert({hash, framebuffer});

		return framebuffer;
	}

	/*
	 */
	bool RenderGraphImpl::removeRenderPass(size_t index)
	{
		if (index >= passes.size())
			return false;

		IRenderPass *pass = passes[index];
		pass->shutdown();
		delete pass;

		passes.erase(passes.begin() + index);
		return true;
	}

	bool RenderGraphImpl::removeRenderPass(IRenderPass *pass)
	{
		auto it = std::find(passes.begin(), passes.end(), pass);
		if (it == passes.end())
			return false;

		size_t index = std::distance(passes.begin(), it);
		return removeRenderPass(index);
	}

	void RenderGraphImpl::removeAllRenderPasses()
	{
		for (IRenderPass *pass : passes)
		{
			pass->shutdown();
			delete pass;
		}

		passes.clear();
	}

	/*
	 */
	size_t RenderGraphImpl::getGroupParameterSize(const char *group_name, const char *parameter_name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(group_name));
		common::HashUtils::combine(hash, std::string_view(parameter_name));

		auto it = group_parameter_lookup.find(hash);
		if (it == group_parameter_lookup.end())
			return 0;

		GroupParameter *parameter = it->second;
		return parameter->size;
	}

	const void *RenderGraphImpl::getGroupParameter(const char *group_name, const char *parameter_name, size_t offset) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(group_name));
		common::HashUtils::combine(hash, std::string_view(parameter_name));

		auto it = group_parameter_lookup.find(hash);
		if (it == group_parameter_lookup.end())
			return nullptr;

		GroupParameter *parameter = it->second;
		const uint8_t *data = reinterpret_cast<const uint8_t *>(parameter->memory);

		if (offset >= parameter->size)
			return nullptr;

		return data + offset;
	}

	bool RenderGraphImpl::setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_offset, size_t src_size, const void *src_data)
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

		if (dst_offset >= parameter->size)
			return false;

		if ((parameter->size - dst_offset) < src_size)
			return false;

		memcpy(dst_data + dst_offset, src_data, src_size);

		group->dirty = true;

		return true;
	}

	/*
	 */
	bool RenderGraphImpl::registerRenderPass(const char *name, PFN_createRenderPass function)
	{
		uint64_t render_pass_hash = 0;
		common::HashUtils::combine(render_pass_hash, std::string_view(name));

		auto it = render_pass_type_lookup.find(render_pass_hash);
		if (it != render_pass_type_lookup.end())
			return false;

		render_pass_type_lookup.insert({render_pass_hash, function});
		return true;
	}

	IRenderPass *RenderGraphImpl::createRenderPass(const char *name)
	{
		uint64_t render_pass_hash = 0;
		common::HashUtils::combine(render_pass_hash, std::string_view(name));

		auto it = render_pass_type_lookup.find(render_pass_hash);
		if (it == render_pass_type_lookup.end())
			return nullptr;

		PFN_createRenderPass function = it->second;
		assert(function);

		IRenderPass *render_pass = function(this);
		assert(render_pass);

		passes.push_back(render_pass);
		return render_pass;
	}

	/*
	 */
	void RenderGraphImpl::destroyGroup(RenderGraphImpl::Group *group)
	{
		assert(group);

		invalidateGroup(group);

		for (GroupParameter *parameter : group->parameters)
			parameter->group = nullptr;

		for (GroupTexture *texture : group->textures)
			texture->group = nullptr;

		delete group;
	}

	void RenderGraphImpl::invalidateGroup(RenderGraphImpl::Group *group)
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

	bool RenderGraphImpl::flushGroup(RenderGraphImpl::Group *group)
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
			if (current_offset > 0 && current_offset + parameter->size > alignment)
				ubo_size += padding;

			ubo_size += static_cast<uint32_t>(parameter->size);
			current_offset = (current_offset + parameter->size) % alignment;
		}

		if (group->buffer_size < ubo_size)
		{
			device->destroyUniformBuffer(group->buffer);
			device->destroyBindSet(group->bindings);

			group->buffer_size = ubo_size;
			group->buffer = device->createUniformBuffer(foundation::render::BufferType::DYNAMIC, ubo_size);

			should_invalidate = true;
		}

		if (group->buffer_size > 0)
		{
			uint8_t *ubo_data = reinterpret_cast<uint8_t *>(device->map(group->buffer));

			current_offset = 0;
			for (GroupParameter *parameter : group->parameters)
			{
				size_t padding = alignment - current_offset % alignment;
				if (current_offset > 0 && current_offset + parameter->size > alignment)
					ubo_data += padding;

				memcpy(ubo_data, parameter->memory, parameter->size);

				ubo_data += parameter->size;
				current_offset = (current_offset + parameter->size) % alignment;
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

		for (GroupTexture *texture : group->textures)
			device->bindTexture(group->bindings, binding++, texture->texture->gpu_data);

		group->dirty = false;
		return should_invalidate;
	}

	/*
	 */
	void RenderGraphImpl::destroyGroupParameter(RenderGraphImpl::GroupParameter *parameter)
	{
		assert(parameter);

		parameter_allocator.deallocate(parameter->memory);
		delete parameter;
	}

	void RenderGraphImpl::destroyGroupTexture(RenderGraphImpl::GroupTexture *texture)
	{
		assert(texture);

		delete texture;
	}

	/*
	 */
	void RenderGraphImpl::destroyRenderBuffer(RenderGraphImpl::RenderBuffer *buffer)
	{
		assert(buffer);

		invalidateRenderBuffer(buffer);

		delete buffer;
	}

	void RenderGraphImpl::invalidateRenderBuffer(RenderGraphImpl::RenderBuffer *buffer)
	{
		assert(buffer);

		device->destroyTexture(buffer->texture);
		buffer->texture = nullptr;

		device->destroyBindSet(buffer->bindings);
		buffer->bindings = nullptr;
	}

	bool RenderGraphImpl::flushRenderBuffer(RenderGraphImpl::RenderBuffer *texture)
	{
		assert(texture);

		if (texture->texture)
			return false;

		assert(texture->bindings == nullptr);

		foundation::render::Format format = texture->format;
		uint32_t texture_width = std::max<uint32_t>(1, width / texture->downscale);
		uint32_t texture_height = std::max<uint32_t>(1, height / texture->downscale);

		texture->texture = device->createTexture2D(texture_width, texture_height, 1, format);
		texture->bindings = device->createBindSet();

		device->bindTexture(texture->bindings, 0, texture->texture);
		return true;
	}

	/*
	 */
	void RenderGraphImpl::invalidateFrameBufferCache()
	{
		for (auto &[hash, framebuffer] : framebuffer_cache)
			device->destroyFrameBuffer(framebuffer);

		framebuffer_cache.clear();
	}
}
