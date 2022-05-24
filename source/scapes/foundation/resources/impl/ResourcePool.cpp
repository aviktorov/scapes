#include "resources/impl/ResourcePool.h"

#include <scapes/foundation/resources/ResourceManager.h>

namespace scapes::foundation::resources::impl
{
	/*
	 */
	ResourcePool::ResourcePool(size_t element_size)
		: element_size(element_size)
	{
		assert(element_size > 0);
	}

	ResourcePool::~ResourcePool()
	{
		for (auto &page : pages)
		{
			assert(page.free_elements_mask == INITIAL_FREE_MASK);
			::free(page.memory);
		}

		pages.clear();

		element_size = 0;
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
			new_page.memory = ::malloc(element_size * ELEMENTS_IN_PAGE);
			memset(new_page.memory, 0, element_size * ELEMENTS_IN_PAGE);
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
				memory = reinterpret_cast<uint8_t*>(page->memory) + element_size * i;
				page->free_elements_mask &= ~mask;
				break;
			}
		}

		assert(memory);
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
			size_t page_end = page_start + ELEMENTS_IN_PAGE * element_size;

			if (page_memory >= page_start && page_memory < page_end)
			{
				assert((page_memory - page_start) % element_size == 0);
				size_t element_index = (page_memory - page_start) / element_size;

				page.free_elements_mask |= static_cast<uint64_t>(1) << element_index;
				break;
			}
		}
	}

	void ResourcePool::clear()
	{
		for (size_t i = 0; i < pages.size(); ++i)
		{
			Page &page = pages[i];
			page.free_elements_mask = INITIAL_FREE_MASK;
		}
	}

	void ResourcePool::traverse(std::function<void (void *)> func)
	{
		for (size_t i = 0; i < pages.size(); ++i)
		{
			Page &page = pages[i];

			for (int j = 0; j < ELEMENTS_IN_PAGE; ++j)
			{
				uint64_t mask = static_cast<uint64_t>(1) << j;
				if (mask & page.free_elements_mask)
					continue;

				void *memory = reinterpret_cast<uint8_t*>(page.memory) + element_size * j;
				func(memory);
			}
		}
	}
}
