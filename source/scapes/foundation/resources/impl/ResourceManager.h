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
		ResourceVTable *fetchVTable(const char *type_name) final;
		void *allocate(const char *type_name, size_t size) final;
		void deallocate(void *memory, const char *type_name) final;

	private:
		ResourcePool *fetchPool(const char *type_name, size_t size);
		ResourcePool *getPool(const char *type_name) const;

	private:
		io::FileSystem *file_system;
		std::map<const char *, ResourcePool *> pools;
		std::map<const char *, ResourceVTable *> vtables;
	};
}
