#include "Resources.h"

/*
 */
ResourcePool::ResourcePool(size_t type_size, size_t type_alignment)
	: type_size(type_size), type_alignment(type_alignment)
{
	stride = type_size;
	stride += sizeof(timestamp_t);
	stride += sizeof(generation_t);

	generation_stride = type_size;
	generation_stride += sizeof(timestamp_t);
}

ResourcePool::~ResourcePool()
{
	for (auto &page : pages)
	{
		assert(page.free_elements_mask == INITIAL_FREE_MASK);
		delete page.memory;
	}

	pages.clear();

	stride = 0;
	type_size = 0;
	type_alignment = 0;
}

/*
 */
void *ResourcePool::allocate()
{
	size_t last_page = pages.size() - 1;

	// TODO: optimize this by adding free list for non-full pages
	//       and iterate over them instead of doing full linear search
	Page *page = nullptr;
	for (size_t i = 0; i < pages.size(); ++i)
	{
		if (!pages[i].isFull())
		{
			page = &pages[i];
			break;
		}
	}

	if (page == nullptr)
	{
		Page new_page;
		new_page.memory = malloc(stride * ELEMENTS_IN_PAGE);
		memset(new_page.memory, 0, stride * ELEMENTS_IN_PAGE);
		new_page.free_elements_mask = INITIAL_FREE_MASK;

		pages.push_back(new_page);
		page = &pages[pages.size() - 1];
	}

	assert(page->memory);
	assert(!page->isFull());

	void *memory = nullptr;

	for (int i = 0; i < ELEMENTS_IN_PAGE; ++i)
	{
		uint64_t mask = static_cast<uint64_t>(1) << i;
		if (mask & page->free_elements_mask)
		{
			memory = reinterpret_cast<uint8_t*>(page->memory) + stride * i;
			page->free_elements_mask &= ~mask;
			break;
		}
	}

	assert(memory);

	timestamp_t *generation = reinterpret_cast<timestamp_t*>(reinterpret_cast<uint8_t*>(memory) + generation_stride);
	(*generation)++;

	return memory;
}

void ResourcePool::deallocate(const void *memory)
{
	size_t page_memory = reinterpret_cast<size_t>(memory);

	// TODO: optimize this by adding binary search tree for all pages
	//       and search in that binary tree instead of doing full linear search
	for (size_t i = 0; i < pages.size(); ++i)
	{
		Page &page = pages[i];

		size_t page_start = reinterpret_cast<size_t>(page.memory);
		size_t page_end = page_start + ELEMENTS_IN_PAGE * stride;

		if (page_memory >= page_start && page_memory < page_end)
		{
			assert((page_memory - page_start) % stride == 0);
			size_t element_index = (page_memory - page_start) / stride;

			page.free_elements_mask |= static_cast<uint64_t>(1) << element_index;
			break;
		}
	}
}

/*
 */
ResourceManagerImpl::ResourceManagerImpl()
{

}

ResourceManagerImpl::~ResourceManagerImpl()
{
	for(auto it : pools)
		delete it.second;

	pools.clear();
}

/*
 */
void *ResourceManagerImpl::allocate(const char *type_name, size_t type_size, size_t type_alignment)
{
	ResourcePool *pool = fetchPool(type_name, type_size, type_alignment);
	assert(pool);

	return pool->allocate();
}

void ResourceManagerImpl::deallocate(void *memory, const char *type_name, size_t type_size, size_t type_alignment)
{
	ResourcePool *pool = fetchPool(type_name, type_size, type_alignment);
	assert(pool);

	pool->deallocate(memory);
}

/*
 */
ResourcePool *ResourceManagerImpl::fetchPool(const char *type_name, size_t type_size, size_t type_alignment)
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
