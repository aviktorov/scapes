#pragma once

#include <volk.h>

namespace scapes::visual::hardware::vulkan
{
	class Context;

	class Platform
	{
	public:
		static const char *getInstanceExtension();
		static VkSurfaceKHR createSurface(const Context *context, void *native_window);
		static void destroySurface(const Context *context, VkSurfaceKHR surface);
	};
}
