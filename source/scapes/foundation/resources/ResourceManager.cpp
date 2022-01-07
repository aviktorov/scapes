#include <resources/impl/ResourceManager.h>

namespace scapes::foundation::resources
{
	ResourceManager *ResourceManager::create(io::FileSystem *file_system)
	{
		return new impl::ResourceManager(file_system);
	}

	void ResourceManager::destroy(ResourceManager *resource_manager)
	{
		delete resource_manager;
	}
}
