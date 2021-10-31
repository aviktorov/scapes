#include "ResourceManager.h"
#include "ResourcePool.h"

namespace scapes::foundation::resources::impl
{
	/*
	 */
	ResourceManager::ResourceManager()
	{

	}

	ResourceManager::~ResourceManager()
	{
		for(auto it : pools)
			delete it.second;

		pools.clear();
	}

	/*
	 */
	void *ResourceManager::allocate(const char *type_name, size_t type_size, size_t type_alignment)
	{
		ResourcePool *pool = fetchPool(type_name, type_size, type_alignment);
		assert(pool);

		return pool->allocate();
	}

	void ResourceManager::deallocate(void *memory, const char *type_name, size_t type_size, size_t type_alignment)
	{
		ResourcePool *pool = fetchPool(type_name, type_size, type_alignment);
		assert(pool);

		pool->deallocate(memory);
	}

	/*
	 */
	ResourcePool *ResourceManager::fetchPool(const char *type_name, size_t type_size, size_t type_alignment)
	{
		ResourcePool *resource_pool = nullptr;

		auto it = pools.find(type_name);
		if (it == pools.end())
		{
			resource_pool = new ResourcePool(type_size, type_alignment);
			pools.insert({type_name, resource_pool});
		}
		else
			resource_pool = it->second;

		return resource_pool;
	}
}
