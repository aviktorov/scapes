#pragma once

#include <scapes/Common.h>
#include <vector>

namespace scapes::foundation::resources::impl
{
	class ResourcePool
	{
	public:
		ResourcePool(size_t type_size, size_t type_alignment);
		virtual ~ResourcePool();

		void *allocate();
		void deallocate(const void *memory);

	private:
		enum : uint64_t
		{
			INITIAL_FREE_MASK = 0xFFFFFFFFFFFFFFFF,
			ELEMENTS_IN_PAGE = 64,
		};

		struct Page
		{
			void *memory {nullptr};
			uint64_t free_elements_mask {INITIAL_FREE_MASK};

			SCAPES_INLINE bool isFull() const { return free_elements_mask == 0; }
		};

		std::vector<Page> pages;

		size_t type_size {0};
		size_t type_alignment {0};
		size_t generation_stride {0};
		size_t stride {0};
	};
}
