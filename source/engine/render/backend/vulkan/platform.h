#pragma once

#include <volk.h>

namespace render::backend::vulkan
{
	class Device;

	class Platform
	{
	public:
		static const char *getInstanceExtension();
		static VkSurfaceKHR createSurface(const render::backend::vulkan::Device *device, void *native_window);
		static void destroySurface(const render::backend::vulkan::Device *device, VkSurfaceKHR surface);
	};
}
