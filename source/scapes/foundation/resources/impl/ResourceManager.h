#pragma once

#include <scapes/foundation/resources/ResourceManager.h>

#include <map>

namespace scapes::foundation::resources::impl
{
	class ResourcePool;

	class ResourceManager : public resources::ResourceManager
	{
	public:
		ResourceManager(io::FileSystem *file_system);
		~ResourceManager() final;

		SCAPES_INLINE io::FileSystem *getFileSystem() const final { return file_system; }
	private:
		void *allocate(const char *type_name, size_t type_size) final;
		void deallocate(void *memory, const char *type_name, size_t type_size) final;

	private:
		ResourcePool *fetchPool(const char *type_name, size_t type_size);

	private:
		io::FileSystem *file_system;
		std::map<const char *, ResourcePool *> pools;
	};
}
