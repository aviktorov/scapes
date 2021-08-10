#include "common/Resources.h"
#include <common/ResourceManager.h>

namespace resources
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
