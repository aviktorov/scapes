#pragma once

#include <scapes/Common.h>
#include <scapes/visual/serde/Yaml.h>

#include <scapes/visual/Texture.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace scapes::visual::impl
{
	class ParameterAllocator
	{
	public:
		void *allocate(size_t size);
		void deallocate(void *memory);
	};

	class GpuBindings
	{
	public:
		GpuBindings(
			foundation::resources::ResourceManager *resource_manager,
			hardware::Device *device
		);
		~GpuBindings();

		void copyTo(GpuBindings &target);

		void clear();
		void invalidate();
		bool flush();

		bool deserialize(const foundation::serde::yaml::NodeRef root);
		bool serialize(foundation::serde::yaml::NodeRef root);

		bool addGroup(const char *name);
		bool removeGroup(const char *name);
		bool clearGroup(const char *name);

		hardware::BindSet getGroupBindings(const char *name) const;

		bool addGroupParameter(const char *group_name, const char *parameter_name, size_t element_size, size_t num_elements);
		bool addGroupParameter(const char *group_name, const char *parameter_name, GroupParameterType type, size_t num_elements);
		bool removeGroupParameter(const char *group_name, const char *parameter_name);

		bool addGroupTexture(const char *group_name, const char *texture_name);
		bool removeGroupTexture(const char *group_name, const char *texture_name);

		size_t getGroupParameterElementSize(const char *group_name, const char *parameter_name) const;
		size_t getGroupParameterNumElements(const char *group_name, const char *parameter_name) const;
		const void *getGroupParameter(const char *group_name, const char *parameter_name, size_t index) const;
		bool setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_index, size_t num_src_elements, const void *src_data);

		TextureHandle getGroupTexture(const char *group_name, const char *texture_name) const;
		bool setGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle);

	private:
		struct Group;
		struct GroupParameter;
		struct GroupTexture;

		struct GroupParameter
		{
			std::string name;
			GroupParameterType type {GroupParameterType::UNDEFINED};
			size_t element_size {0};
			size_t num_elements {0};
			void *memory {nullptr};

			Group *group {nullptr};
		};

		struct GroupTexture
		{
			std::string name;
			TextureHandle texture;

			Group *group {nullptr};
		};

		struct Group
		{
			std::string name;
			std::vector<GroupParameter *> parameters;
			std::vector<GroupTexture *> textures;

			hardware::BindSet bindings {SCAPES_NULL_HANDLE};
			hardware::UniformBuffer buffer {SCAPES_NULL_HANDLE};
			uint32_t buffer_size {0};
			bool dirty {true};
		};

	private:
		bool addGroupParameterInternal(const char *group_name, const char *parameter_name, GroupParameterType type, size_t element_size, size_t num_elements);

		void clearGroup(Group *group);
		void invalidateGroup(Group *group);
		bool flushGroup(Group *group);

		void deserializeGroup(foundation::serde::yaml::NodeRef group_node);
		void deserializeGroupParameter(const char *group_name, foundation::serde::yaml::NodeRef parameter_node);
		void deserializeGroupTexture(const char *group_name, foundation::serde::yaml::NodeRef texture_node);

	private:
		foundation::resources::ResourceManager *resource_manager {nullptr};
		hardware::Device *device {nullptr};

		ParameterAllocator parameter_allocator;

		std::unordered_map<uint64_t, Group *> group_lookup;
		std::unordered_map<uint64_t, GroupParameter *> group_parameter_lookup;
		std::unordered_map<uint64_t, GroupTexture *> group_texture_lookup;
	};
}
