#pragma once

#include <scapes/foundation/resources/ResourceManager.h>

#include <unordered_map>

namespace scapes::foundation::resources::impl
{
	class ResourcePool;

	class ResourceManager : public resources::ResourceManager
	{
	public:
		ResourceManager(io::FileSystem *file_system);
		~ResourceManager() final;

		SCAPES_INLINE io::FileSystem *getFileSystem() const final { return file_system; }
		void update(float dt) final;

	private:
		bool linkMemory(void *memory, const io::URI &uri) final;
		bool unlinkMemory(void *memory) final;
		void *getLinkedMemory(const io::URI &uri) const final;
		io::URI getLinkedUri(void *memory) const final;

		ResourceVTable *fetchVTable(const char *type_name) final;
		void *allocate(const char *type_name, size_t size) final;
		void deallocate(void *memory, const char *type_name) final;

	private:
		ResourcePool *fetchPool(const char *type_name, size_t size);
		ResourcePool *getPool(const char *type_name) const;

	private:
		io::FileSystem *file_system {nullptr};
		std::unordered_map<size_t, ResourcePool *> pools;
		std::unordered_map<size_t, ResourceVTable *> vtables;

		std::unordered_map<size_t, io::URI> uri_by_resource;
		std::unordered_map<size_t, std::vector<void *>> resources_by_uri;
	};
}
