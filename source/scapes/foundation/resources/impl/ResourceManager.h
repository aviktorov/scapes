#pragma once

#include <scapes/foundation/resources/ResourceManager.h>

#include <map>

namespace scapes::foundation::resources::impl
{
	class ResourcePool;

	class ResourceManager : public resources::ResourceManager
	{
	public:
		ResourceManager();
		~ResourceManager() final;

	private:
		void *allocate(const char *type_name, size_t type_size, size_t type_alignment) final;
		void deallocate(void *memory, const char *type_name, size_t type_size, size_t type_alignment) final;

	private:
		ResourcePool *fetchPool(const char *type_name, size_t type_size, size_t type_alignment);

	private:
		std::map<const char *, ResourcePool *> pools;
	};
}
