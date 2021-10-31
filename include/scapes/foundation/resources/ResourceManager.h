#pragma once

#include <scapes/Common.h>
#include <scapes/foundation/Log.h>
#include <scapes/foundation/TypeTraits.h>
#include <scapes/foundation/Fwd.h>

template <typename T> struct ResourcePipeline { };

namespace scapes::foundation::resources
{
	// TODO: better types
	typedef int timestamp_t;
	typedef int generation_t;

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

		T *operator->() const { return get(); }

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
		ResourceHandle<T> create()
		{
			void *memory = allocate(TypeTraits<T>::name, sizeof(T), alignof(T));
			assert(memory);
			new (memory) T();

			return ResourceHandle<T>(memory);
		}

		template <typename T, typename... Arguments>
		ResourceHandle<T> import(const io::URI &uri, Arguments&&... params)
		{
			// TODO: use I/O to load resource into temp memory and provide to resource pipeline
			FILE *file = fopen(uri, "rb");
			if (!file)
			{
				Log::error("ResourceManager::import(): can't open \"%s\" file\n", uri);
				return ResourceHandle<T>();
			}

			fseek(file, 0, SEEK_END);
			size_t size = ftell(file);
			fseek(file, 0, SEEK_SET);

			uint8_t *data = new uint8_t[size];
			fread(data, sizeof(uint8_t), size, file);
			fclose(file);

			ResourceHandle<T> resource = create<T>();
			ResourcePipeline<T>::process(this, resource, data, size, std::forward<Arguments>(params)...);
			delete[] data;

			return resource;
		}

		template <typename T, typename... Arguments>
		ResourceHandle<T> importFromMemory(const uint8_t *data, size_t size, Arguments&&... params)
		{
			ResourceHandle<T> resource = create<T>();
			ResourcePipeline<T>::process(this, resource, data, size, std::forward<Arguments>(params)...);

			return resource;
		}

		template <typename T, typename... Arguments>
		void destroy(ResourceHandle<T> resource, Arguments&&... params)
		{
			ResourcePipeline<T>::destroy(this, resource, std::forward<Arguments>(params)...);
			resource.get()->~T();

			deallocate(resource.get(), TypeTraits<T>::name, sizeof(T), alignof(T));
		}

	protected:
		virtual void *allocate(const char *type_name, size_t type_size, size_t type_alignment) = 0;
		virtual void deallocate(void *memory, const char *type_name, size_t type_size, size_t type_alignment) = 0;
	};
}
