#include <hardware/vulkan/Device.h>

namespace scapes::visual::hardware
{
	Device *Device::create(const char *application_name, const char *engine_name, Api api)
	{
		switch (api)
		{
			case Api::VULKAN: return new vulkan::Device(application_name, engine_name);
		}

		return nullptr;
	}

	void Device::destroy(Device *device)
	{
		delete device;
	}
}
