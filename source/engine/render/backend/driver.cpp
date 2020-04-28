#include "render/backend/driver.h"
#include "render/backend/vulkan/driver.h"

#include <cassert>

namespace render::backend
{
	Driver *Driver::create(const char *application_name, const char *engine_name, Api api)
	{
		switch (api)
		{
			case Api::VULKAN: return new VulkanDriver(application_name, engine_name);
		}

		return nullptr;
	}
}
