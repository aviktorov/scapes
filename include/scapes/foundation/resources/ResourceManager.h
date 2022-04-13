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
	typedef uint64_t hash_t;
	typedef uint32_t generation_t;

	struct ResourceMetadata
	{
		generation_t generation {0};
		hash_t hash {0};
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

		SCAPES_INLINE bool isValid() const
		{
			if (!memory)
				return false;

			if (generation != getMemoryGeneration())
				return false;

			return true;
		}

		SCAPES_INLINE T *get() const
		{
			if (!isValid())
				return nullptr;

			return reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(memory) + sizeof(ResourceMetadata));
		}

		SCAPES_INLINE void *getRaw() const { return memory; }
		SCAPES_INLINE generation_t getGeneration() const { return generation; }

		hash_t getHash() const
		{
			if (!memory)
				return 0;

			const ResourceMetadata *metadata = reinterpret_cast<const ResourceMetadata *>(memory);
			return metadata->hash;
		}

		T *operator->() const { return get(); }

	private:
		friend class ResourceManager;

		ResourceHandle(void *memory)
			: memory(memory)
		{
			generation = getMemoryGeneration();
		}

		SCAPES_INLINE generation_t getMemoryGeneration() const
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
		virtual void update(float dt) = 0;

	public:
		template <typename T, typename... Arguments>
		ResourceHandle<T> create(Arguments &&...params)
		{
			size_t size = ResourceTraits<T>::size() + sizeof(ResourceMetadata);
			void *memory = allocate(TypeTraits<T>::name, size);
			assert(memory);

			ResourceMetadata *meta = reinterpret_cast<ResourceMetadata *>(memory);
			meta->generation++;
			meta->hash = 0;
			meta->type_name = TypeTraits<T>::name;

			ResourceVTable *vtable = fetchVTable(TypeTraits<T>::name);
			assert(vtable);

			vtable->destroy = ResourceTraits<T>::destroy;
			vtable->reload = ResourceTraits<T>::reload;
			vtable->fetchHash = ResourceTraits<T>::fetchHash;
			vtable->offset = sizeof(ResourceMetadata);

			void *resource_memory = reinterpret_cast<uint8_t *>(memory) + sizeof(ResourceMetadata);
			assert(resource_memory);

			ResourceTraits<T>::create(this, resource_memory, std::forward<Arguments>(params)...);

			return ResourceHandle<T>(memory);
		}

		template <typename T>
		bool link(const ResourceHandle<T> &handle, const io::URI &uri)
		{
			return linkMemory(handle.getRaw(), uri);
		}

		template <typename T>
		bool unlink(const ResourceHandle<T> &handle)
		{
			return unlinkMemory(handle.getRaw());
		}

		template <typename T>
		io::URI getUri(const ResourceHandle<T> &handle)
		{
			return getLinkedUri(handle.getRaw());
		}

		template <typename T, typename... Arguments>
		ResourceHandle<T> fetch(const io::URI &uri, Arguments &&...params)
		{
			if (void *memory = getLinkedMemory(uri); memory)
				return ResourceHandle<T>(memory);

			return load<T, Arguments...>(uri, std::forward<Arguments>(params)...);
		}

		template <typename T, typename... Arguments>
		ResourceHandle<T> load(const io::URI &uri, Arguments &&...params)
		{
			io::FileSystem *file_system = getFileSystem();
			assert(file_system);

			size_t size = 0;
			uint8_t *data = reinterpret_cast<uint8_t *>(file_system->map(uri, size));

			if (!data)
			{
				Log::error("ResourceManager::load(): can't open \"%s\" file\n", uri);
				return ResourceHandle<T>();
			}

			ResourceHandle<T> resource = loadFromMemory<T>(data, size, std::forward<Arguments>(params)...);
			file_system->unmap(data);

			const ResourceMetadata *metadata = reinterpret_cast<ResourceMetadata *>(resource.getRaw());
			assert(metadata);

			ResourceVTable *vtable = fetchVTable(metadata->type_name);
			hash_t hash = vtable->fetchHash(this, file_system, resource.get(), uri);

			setHash(resource.getRaw(), hash);
			linkMemory(resource.getRaw(), uri);

			return resource;
		}

		template <typename T, typename... Arguments>
		ResourceHandle<T> loadFromMemory(const uint8_t *data, size_t size, Arguments &&...params)
		{
			ResourceHandle<T> resource = create<T, Arguments...>(std::forward<Arguments>(params)...);
			ResourceTraits<T>::loadFromMemory(this, resource.get(), data, size);

			return resource;
		}

		template <typename T>
		void flushToGPU(ResourceHandle<T> resource)
		{
			ResourceTraits<T>::flushToGPU(this, resource.get());
		}

		template <typename T>
		void destroy(ResourceHandle<T> resource)
		{
			ResourceMetadata *metadata = reinterpret_cast<ResourceMetadata *>(resource.getRaw());
			assert(metadata);

			ResourceVTable *vtable = fetchVTable(metadata->type_name);

			T *resource_memory = resource.get();
			assert(resource_memory);

			vtable->destroy(this, resource_memory);

			deallocate(resource.getRaw(), TypeTraits<T>::name);
		}

	protected:
		struct ResourceVTable
		{
			using DestroyFuncPtr = void (*)(ResourceManager *, void *);
			using ReloadFuncPtr = bool (*)(ResourceManager *, io::FileSystem *, void *, const io::URI &);
			using FetchHashFuncPtr = hash_t (*)(ResourceManager *, io::FileSystem *, void *, const io::URI &);

			DestroyFuncPtr destroy {nullptr};
			ReloadFuncPtr reload {nullptr};
			FetchHashFuncPtr fetchHash {nullptr};
			size_t offset {0};
		};

		SCAPES_INLINE static void setHash(void *memory, hash_t hash)
		{
			ResourceMetadata *metadata = reinterpret_cast<ResourceMetadata *>(memory);
			assert(metadata);

			metadata->hash = hash;
		}

		SCAPES_INLINE static hash_t getHash(void *memory)
		{
			ResourceMetadata *metadata = reinterpret_cast<ResourceMetadata *>(memory);
			assert(metadata);

			return metadata->hash;
		}

		SCAPES_INLINE static const char *getTypeName(void *memory)
		{
			ResourceMetadata *metadata = reinterpret_cast<ResourceMetadata *>(memory);
			assert(metadata);

			return metadata->type_name;
		}

		virtual bool linkMemory(void *memory, const io::URI &uri) = 0;
		virtual bool unlinkMemory(void *memory) = 0;
		virtual void *getLinkedMemory(const io::URI &uri) const = 0;
		virtual io::URI getLinkedUri(void *memory) const = 0;

		virtual ResourceVTable *fetchVTable(const char *type_name) = 0;
		virtual void *allocate(const char *type_name, size_t size) = 0;
		virtual void deallocate(void *memory, const char *type_name) = 0;
	};
}
