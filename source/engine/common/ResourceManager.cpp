#include <common/ResourceManager.h>
#include "common/Resources.h"

ResourceManager *ResourceManager::create()
{
	return new ResourceManagerImpl();
}

void ResourceManager::destroy(ResourceManager *resource_manager)
{
	delete resource_manager;
}
