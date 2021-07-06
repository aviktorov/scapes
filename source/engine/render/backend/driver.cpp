#include <render/backend/Driver.h>
#include "render/backend/vulkan/Driver.h"

#include <cassert>

namespace render::backend
{
	Driver *Driver::create(const char *application_name, const char *engine_name, Api api)
	{
		switch (api)
		{
			case Api::VULKAN: return new vulkan::Driver(application_name, engine_name);
		}

		return nullptr;
	}
}
