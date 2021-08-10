#pragma once

#include <common/ResourceManager.h>

#include <vector>
#include <map>

namespace resources::impl
{
	class ResourcePool
	{
	public:
		ResourcePool(size_t type_size, size_t type_alignment)
			: type_size(type_size), type_alignment(type_alignment)
		{
			stride = type_size;
			stride += sizeof(timestamp_t);
			stride += sizeof(generation_t);
		}

		virtual ~ResourcePool()
		{
			for (auto &page : pages)
				delete page.memory;

			pages.clear();
			resource_map.clear();

			stride = 0;
			type_size = 0;
			type_alignment = 0;
		}

		void *get(const ResourceID &id) const
		{
			auto it = resource_map.find(id);
			if (it != resource_map.end())
				return it->second;

			return nullptr;
		}

		void *allocate(const ResourceID &id)
		{
			assert(get(id) == nullptr);
			size_t last_page = pages.size() - 1;

			Page page;
			if (pages.size() == 0 || pages[last_page].isFull())
			{
				page.memory = malloc(stride * ELEMENTS_IN_PAGE);
				memset(page.memory, 0, stride * ELEMENTS_IN_PAGE);
				page.num_elements = ELEMENTS_IN_PAGE;
				page.allocated_elements = 0;

				pages.push_back(page);
			}
			else
				page = pages[last_page];

			assert(page.memory);
			assert(page.allocated_elements < page.num_elements);

			void *memory = reinterpret_cast<uint8_t*>(page.memory) + page.allocated_elements++;
			resource_map.insert({id, memory});

			return memory;
		}

	private:
		struct Page
		{
			void *memory {nullptr};
			size_t num_elements {0};
			size_t allocated_elements {0};

			SCAPES_INLINE bool isFull() const { return allocated_elements == num_elements; }
		};

		enum
		{
			ELEMENTS_IN_PAGE = 128,
		};

		std::vector<Page> pages;
		std::map<ResourceID, void*> resource_map;

		size_t type_size {0};
		size_t type_alignment {0};
		size_t stride {0};
	};

	class ResourceManager : public resources::ResourceManager
	{
	private:
		void *get(const ResourceID &id, const char *type_name, size_t type_size, size_t type_alignment) final
		{
			ResourcePool *pool = fetchPool(type_name, type_size, type_alignment);
			assert(pool);

			return pool->get(id);
		}

		void *allocate(const ResourceID &id, const char *type_name, size_t type_size, size_t type_alignment) final
		{
			ResourcePool *pool = fetchPool(type_name, type_size, type_alignment);
			assert(pool);

			return pool->allocate(id);
		}

	private:
		ResourcePool *fetchPool(const char *type_name, size_t type_size, size_t type_alignment)
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

	private:
		std::map<const char *, ResourcePool *> pools;
	};
}
