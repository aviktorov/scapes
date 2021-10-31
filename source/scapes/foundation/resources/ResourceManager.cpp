#include <resources/impl/ResourceManager.h>

namespace scapes::foundation::resources
{
	ResourceManager *ResourceManager::create()
	{
		return new impl::ResourceManager();
	}

	void ResourceManager::destroy(ResourceManager *resource_manager)
	{
		delete resource_manager;
	}
}
