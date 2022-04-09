#pragma once

#include <scapes/foundation/resources/ResourceManager.h>
#include "HashUtils.h"

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

		void *allocate(const char *type_name, size_t size) final;
		void deallocate(void *memory, const char *type_name) final;

	private:
		ResourceVTable *fetchVTable(const char *type_name) final;
		ResourceVTable *getVTable(const char *type_name);

		ResourcePool *fetchPool(const char *type_name, size_t size);
		ResourcePool *getPool(const char *type_name) const;

	private:
		struct ResourceEntry
		{
			void *memory {nullptr};
			ResourceVTable *vtable {nullptr};
		};

		struct URIHasher
		{
			std::size_t operator()(const io::URI &uri) const
			{
				uint64_t hash = 0;
				common::HashUtils::combine(hash, std::string_view(uri.c_str()));

				return static_cast<std::size_t>(hash);
			}
		};

	private:
		io::FileSystem *file_system {nullptr};
		std::unordered_map<size_t, ResourcePool *> pools;
		std::unordered_map<size_t, ResourceVTable *> vtables;

		std::unordered_map<size_t, io::URI> uri_by_resource;
		std::unordered_map<io::URI, std::vector<ResourceEntry>, URIHasher> resources_by_uri;
	};
}
