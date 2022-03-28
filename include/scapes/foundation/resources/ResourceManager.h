#pragma once

#include <scapes/Common.h>
#include <scapes/foundation/Log.h>
#include <scapes/foundation/TypeTraits.h>
#include <scapes/foundation/Fwd.h>
#include <scapes/foundation/io/FileSystem.h>

template <typename T> struct ResourceTraits { };

namespace scapes::foundation::resources
{
	// TODO: better types
	typedef uint32_t timestamp_t;
	typedef uint32_t generation_t;

	struct ResourceMetadata
	{
		generation_t generation {0};
		timestamp_t timestamp {0};
		const char *type_name {nullptr};
	};

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

			return reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(memory) + sizeof(ResourceMetadata));
		}

		SCAPES_INLINE void *getMemory() const { return memory; }

		timestamp_t getMTime() const
		{
			if (!memory)
				return 0;

			const ResourceMetadata *metadata = reinterpret_cast<const ResourceMetadata *>(memory);
			return metadata->timestamp;
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

			const ResourceMetadata *metadata = reinterpret_cast<const ResourceMetadata *>(memory);
			return metadata->generation;
		}

	private:
		void *memory {nullptr};
		generation_t generation {0};
	};

	class ResourceManager
	{
	public:
		static SCAPES_API ResourceManager *create(io::FileSystem *file_system);
		static SCAPES_API void destroy(ResourceManager *resource_manager);

		virtual ~ResourceManager() { }

		virtual io::FileSystem *getFileSystem() const = 0;

	public:
		template <typename T, typename... Arguments>
		ResourceHandle<T> create(Arguments &&...params)
		{
			size_t size = ResourceTraits<T>::size() + sizeof(ResourceMetadata);
			void *memory = allocate(TypeTraits<T>::name, size);
			assert(memory);

			ResourceMetadata *meta = reinterpret_cast<ResourceMetadata *>(memory);
			meta->generation++;
			meta->timestamp = 0;
			meta->type_name = TypeTraits<T>::name;

			ResourceVTable *vtable = fetchVTable(TypeTraits<T>::name);
			assert(vtable);

			vtable->destroy = ResourceTraits<T>::destroy;

			void *resource_memory = reinterpret_cast<uint8_t *>(memory) + sizeof(ResourceMetadata);
			assert(resource_memory);

			ResourceTraits<T>::create(this, resource_memory, std::forward<Arguments>(params)...);

			return ResourceHandle<T>(memory);
		}

		template <typename T, typename... Arguments>
		ResourceHandle<T> import(const io::URI &uri, Arguments &&...params)
		{
			io::FileSystem *file_system = getFileSystem();
			assert(file_system);

			io::Stream *stream = file_system->open(uri, "rb");
			if (!stream)
			{
				Log::error("ResourceManager::import(): can't open \"%s\" file\n", uri);
				return ResourceHandle<T>();
			}

			size_t size = static_cast<size_t>(stream->size());

			uint8_t *data = new uint8_t[size];
			stream->read(data, sizeof(uint8_t), size);
			file_system->close(stream);

			ResourceHandle<T> resource = importFromMemory<T>(data, size, std::forward<Arguments>(params)...);
			delete[] data;

			return resource;
		}

		template <typename T, typename... Arguments>
		ResourceHandle<T> importFromMemory(const uint8_t *data, size_t size, Arguments &&...params)
		{
			ResourceHandle<T> resource = create<T, Arguments...>(std::forward<Arguments>(params)...);
			ResourceTraits<T>::importFromMemory(this, resource.get(), data, size);

			return resource;
		}

		template <typename T>
		void destroy(ResourceHandle<T> resource)
		{
			ResourceVTable *vtable = fetchVTable(TypeTraits<T>::name);
			T *resource_memory = resource.get();
			assert(resource_memory);

			vtable->destroy(this, resource_memory);

			deallocate(resource.getMemory(), TypeTraits<T>::name);
		}

	protected:
		struct ResourceVTable
		{
			using DestroyFuncPtr = void (*)(ResourceManager *, void *);

			DestroyFuncPtr destroy {nullptr};
		};

		virtual ResourceVTable *fetchVTable(const char *type_name) = 0;
		virtual void *allocate(const char *type_name, size_t size) = 0;
		virtual void deallocate(void *memory, const char *type_name) = 0;
	};
}
