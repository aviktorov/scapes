#pragma once

#include <common/Common.h>
#include <common/Type.h>

namespace resources
{
	typedef int timestamp_t;
	typedef int generation_t;

	typedef const char * ResourceID;

	template <typename T>
	struct ResourceTraits { };

	template <typename T>
	struct ResourcePipeline { };

	template <typename T>
	class ResourceHandle
	{
	public:
		ResourceHandle()
			: memory(nullptr), generation(0)
		{

		}

		ResourceHandle(const ResourceHandle<T> &res)
			: memory(res.memory), generation(res.generation)
		{

		}

		SCAPES_INLINE T *get() const
		{
			if (!memory)
				return nullptr;

			if (generation != getGeneration())
				return nullptr;

			return reinterpret_cast<T*>(memory);
		}

		timestamp_t getMTime() const
		{
			if (!memory)
				return 0;

			const uint8_t *data = reinterpret_cast<const uint8_t *>(memory) + sizeof(T);
			return *reinterpret_cast<const timestamp_t *>(data);
		}

	private:
		friend class ResourceManager;

		ResourceHandle(void *memory)
			: memory(memory)
		{
			generation = getGeneration();
		}

		SCAPES_INLINE generation_t getGeneration() const
		{
			if (!memory)
				return 0;

			const uint8_t *data = reinterpret_cast<const uint8_t *>(memory) + sizeof(T) + sizeof(timestamp_t);
			return *reinterpret_cast<const generation_t *>(data);
		}

	private:
		void *memory {nullptr};
		generation_t generation {0};
	};

	class ResourceManager
	{
	public:
		static SCAPES_API ResourceManager *create();
		static SCAPES_API void destroy(ResourceManager *resource_manager);

		virtual ~ResourceManager() { }

	public:
		template <typename T>
		ResourceHandle<T> fetch(const ResourceID &id)
		{
			void *memory = get(id, TypeTraits<T>::name, sizeof(T), alignof(T));
			
			T *obj = nullptr;
			if (!memory)
			{
				memory = allocate(id, TypeTraits<T>::name, sizeof(T), alignof(T));
				assert(memory);
				obj = new (memory) T();
			}

			return ResourceHandle<T>(memory);
		}

		template <typename T, typename... Arguments>
		ResourceHandle<T> import(const ResourceID &id, Arguments&&... params)
		{
			ResourceHandle<T> resource = fetch<T>(id);
			ResourcePipeline<T>::import(id, resource, std::forward<Arguments>(params)...);

			return resource;
		}

	protected:
		virtual void *get(const ResourceID &id, const char *type_name, size_t type_size, size_t type_alignment) = 0;
		virtual void *allocate(const ResourceID &id, const char *type_name, size_t type_size, size_t type_alignment) = 0;
	};
}