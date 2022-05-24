#pragma once

#include <scapes/Common.h>

#include <scapes/visual/serde/Yaml.h>
#include <scapes/visual/GroupParameterType.h>
#include <scapes/visual/Texture.h>
#include <scapes/visual/Fwd.h>

namespace scapes::visual
{
	/*
	 */
	class Material
	{
	public:
		virtual ~Material() { }

	public:
		virtual foundation::resources::ResourceManager *getResourceManager() const = 0;
		virtual hardware::Device *getDevice() const = 0;
		virtual shaders::Compiler *getCompiler() const = 0;

		virtual MaterialHandle clone() = 0;
		virtual bool flush() = 0;

		virtual bool deserialize(const foundation::serde::yaml::Tree &tree) = 0;
		virtual foundation::serde::yaml::Tree serialize() = 0;

		virtual bool addGroup(const char *name) = 0;
		virtual bool removeGroup(const char *name) = 0;
		virtual void removeAllGroups() = 0;

		virtual hardware::BindSet getGroupBindings(const char *name) const = 0;

		virtual bool addGroupParameter(const char *group_name, const char *parameter_name, size_t element_size, size_t num_elements) = 0;
		virtual bool addGroupParameter(const char *group_name, const char *parameter_name, GroupParameterType type, size_t num_elements) = 0;
		virtual bool removeGroupParameter(const char *group_name, const char *parameter_name) = 0;

		virtual bool addGroupTexture(const char *group_name, const char *texture_name) = 0;
		virtual bool removeGroupTexture(const char *group_name, const char *texture_name) = 0;

		virtual TextureHandle getGroupTexture(const char *group_name, const char *texture_name) const = 0;
		virtual bool setGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle) = 0;

	public:
		template<typename T>
		SCAPES_INLINE bool addGroupParameter(const char *group_name, const char *parameter_name, size_t num_elements, const T *value)
		{
			bool success = addGroupParameter(group_name, parameter_name, sizeof(T), num_elements);

			if (success)
				return setGroupParameter<T>(group_name, parameter_name, 1, value);

			return success;
		}

		template<typename T>
		SCAPES_INLINE bool addGroupParameter(const char *group_name, const char *parameter_name, const T &value)
		{
			return addGroupParameter(group_name, parameter_name, 1, &value);
		}

		template<typename T>
		SCAPES_INLINE T getGroupParameter(const char *group_name, const char *parameter_name) const
		{
			const T *data = getGroupParameter<T>(group_name, parameter_name, 0);
			return *data;
		}

		template<typename T>
		SCAPES_INLINE const T *getGroupParameter(const char *group_name, const char *parameter_name, size_t index) const
		{
			assert(getGroupParameterElementSize(group_name, parameter_name) == sizeof(T));
			assert(getGroupParameterNumElements(group_name, parameter_name) > index);

			const void *data = getGroupParameter(group_name, parameter_name, index);
			assert(data);

			return reinterpret_cast<const T*>(data);
		}

		template<typename T>
		SCAPES_INLINE bool setGroupParameter(const char *group_name, const char *parameter_name, size_t num_elements, const T *value)
		{
			assert(getGroupParameterElementSize(group_name, parameter_name) == sizeof(T));
			assert(getGroupParameterNumElements(group_name, parameter_name) >= num_elements);

			return setGroupParameter(group_name, parameter_name, 0, num_elements, value);
		}

		template<typename T>
		SCAPES_INLINE bool setGroupParameter(const char *group_name, const char *parameter_name, const T &value)
		{
			return setGroupParameter<T>(group_name, parameter_name, 1, &value);
		}

		SCAPES_INLINE bool addGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle)
		{
			bool success = addGroupTexture(group_name, texture_name);

			if (success)
				return setGroupTexture(group_name, texture_name, handle);

			return success;
		}

	protected:
		virtual size_t getGroupParameterElementSize(const char *group_name, const char *parameter_name) const = 0;
		virtual size_t getGroupParameterNumElements(const char *group_name, const char *parameter_name) const = 0;
		virtual const void *getGroupParameter(const char *group_name, const char *parameter_name, size_t index) const = 0;
		virtual bool setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_index, size_t num_src_elements, const void *src_data) = 0;
	};

	template <>
	struct ::TypeTraits<Material>
	{
		static constexpr const char *name = "scapes::visual::Material";
	};

	template <>
	struct ::ResourceTraits<Material>
	{
		static SCAPES_API size_t size();
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			hardware::Device *device,
			shaders::Compiler *compiler
		);
		static SCAPES_API void destroy(
			foundation::resources::ResourceManager *resource_manager,
			void *memory
		);
		static SCAPES_API foundation::resources::hash_t fetchHash(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API bool reload(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API bool loadFromMemory(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			const uint8_t *data,
			size_t size
		);
	};
}
