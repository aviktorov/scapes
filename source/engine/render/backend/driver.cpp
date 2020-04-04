#include "render/backend/driver.h"
#include "render/backend/vulkan/driver.h"

#include <cassert>

namespace render::backend
{
	Driver *createDriver(const char *application_name, const char *engine_name, Api api)
	{
		switch (api)
		{
			case Api::VULKAN: return new VulkanDriver(application_name, engine_name);
		}

		return nullptr;
	}

	void destroyDriver(Driver *driver)
	{
		assert(driver != nullptr && "Invalid driver");

		delete driver;
		driver = nullptr;
	}
}
