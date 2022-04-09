#include "ResourceManager.h"
#include "ResourcePool.h"

#include <scapes/foundation/profiler/Profiler.h>

namespace scapes::foundation::resources::impl
{
	/*
	 */
	ResourceManager::ResourceManager(io::FileSystem *file_system)
		: file_system(file_system)
	{

	}

	ResourceManager::~ResourceManager()
	{
		for (auto it : pools)
		{
			auto vtable_it = vtables.find(it.first);
			assert(vtable_it != vtables.end());

			ResourceVTable *vtable = vtable_it->second;
			assert(vtable);

			it.second->traverse(
				[this, vtable](void *memory)
				{
					uint8_t *resource_ptr = reinterpret_cast<uint8_t *>(memory) + vtable->offset;
					vtable->destroy(this, resource_ptr);
				}
			);
			it.second->clear();
			delete it.second;
		}

		for (auto it : vtables)
			delete it.second;

		pools.clear();
		vtables.clear();

		uri_by_resource.clear();
		resources_by_uri.clear();
	}

	/*
	 */
	void ResourceManager::update(float dt)
	{
		SCAPES_PROFILER();

		// TODO: memory limit management

		// live reload
		const uint32_t bucket = frame % max_check_frames;

		for (auto it : pools)
		{
			auto vtable_it = vtables.find(it.first);
			assert(vtable_it != vtables.end());

			ResourceVTable *vtable = vtable_it->second;
			assert(vtable);

			int counter = 0;

			it.second->traverse(
				[this, vtable, bucket, &counter](void *memory)
				{
					uint64_t memory_hash = 0;
					common::HashUtils::combine(memory_hash, memory);

					auto it = uri_by_resource.find(memory_hash);
					if (it == uri_by_resource.end())
						return;

					if (bucket != (counter++ % max_check_frames))
						return;

					const io::URI &uri = it->second;

					uint8_t *resource_ptr = reinterpret_cast<uint8_t *>(memory) + vtable->offset;

					hash_t resource_hash = ResourceManager::getHash(memory);
					hash_t file_hash = vtable->fetchHash(this, file_system, resource_ptr, uri);

					if (file_hash == resource_hash)
						return;

					vtable->reload(this, file_system, resource_ptr, uri);
					ResourceManager::setHash(memory, file_hash);
				}
			);
		}

		frame++;
	}

	/*
	 */
	bool ResourceManager::linkMemory(void *memory, const io::URI &uri)
	{
		if (memory == nullptr)
			return false;

		uint64_t memory_hash = 0;
		common::HashUtils::combine(memory_hash, memory);

		auto it = uri_by_resource.find(memory_hash);
		if (it != uri_by_resource.end())
		{
			const io::URI &current_uri = it->second;
			if (uri == current_uri)
				return false;

			unlinkMemory(memory);
		}

		const char *type_name = ResourceManager::getTypeName(memory);

		ResourceEntry entry = {};
		entry.memory = memory;
		entry.vtable = getVTable(type_name);

		assert(entry.memory);
		assert(entry.vtable);

		resources_by_uri[uri].push_back(entry);
		uri_by_resource[memory_hash] = uri;

		return true;
	}

	bool ResourceManager::unlinkMemory(void *memory)
	{
		if (memory == nullptr)
			return false;

		uint64_t memory_hash = 0;
		common::HashUtils::combine(memory_hash, memory);

		auto uri_it = uri_by_resource.find(memory_hash);

		if (uri_it == uri_by_resource.end())
			return false;

		const io::URI &uri = uri_it->second;

		auto resource_it = resources_by_uri.find(uri);
		assert(resource_it != resources_by_uri.end());

		std::vector<ResourceEntry> &resources = resource_it->second;

		for (auto it = resources.begin(); it != resources.end(); ++it)
		{
			if (it->memory == memory)
			{
				resources.erase(it);
				break;
			}
		}

		if (resources.empty())
			resources_by_uri.erase(resource_it);

		uri_by_resource.erase(memory_hash);

		return true;
	}

	void *ResourceManager::getLinkedMemory(const io::URI &uri) const
	{
		auto it = resources_by_uri.find(uri);
		if (it == resources_by_uri.end())
			return nullptr;

		return it->second.front().memory;
	}

	io::URI ResourceManager::getLinkedUri(void *memory) const
	{
		uint64_t memory_hash = 0;
		common::HashUtils::combine(memory_hash, memory);

		auto it = uri_by_resource.find(memory_hash);
		if (it == uri_by_resource.end())
			return nullptr;

		return it->second;
	}

	/*
	 */
	void *ResourceManager::allocate(const char *type_name, size_t type_size)
	{
		ResourcePool *pool = fetchPool(type_name, type_size);
		assert(pool);

		return pool->allocate();
	}

	void ResourceManager::deallocate(void *memory, const char *type_name)
	{
		ResourcePool *pool = getPool(type_name);
		assert(pool);

		pool->deallocate(memory);
		unlinkMemory(memory);
	}

	/*
	 */
	ResourceManager::ResourceVTable *ResourceManager::fetchVTable(const char *type_name)
	{
		ResourceVTable *result = nullptr;

		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(type_name));

		auto it = vtables.find(hash);
		if (it == vtables.end())
		{
			result = new ResourceVTable();
			vtables.insert({hash, result});
		}
		else
			result = it->second;

		return result;
	}

	ResourceManager::ResourceVTable *ResourceManager::getVTable(const char *type_name)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(type_name));

		auto it = vtables.find(hash);
		if (it == vtables.end())
			return nullptr;

		return it->second;
	}

	/*
	 */
	ResourcePool *ResourceManager::fetchPool(const char *type_name, size_t type_size)
	{
		ResourcePool *resource_pool = nullptr;

		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(type_name));

		auto it = pools.find(hash);
		if (it == pools.end())
		{
			resource_pool = new ResourcePool(type_size);
			pools.insert({hash, resource_pool});
		}
		else
			resource_pool = it->second;

		return resource_pool;
	}

	ResourcePool *ResourceManager::getPool(const char *type_name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(type_name));

		auto it = pools.find(hash);
		if (it == pools.end())
			return nullptr;

		return it->second;
	}
}
