#pragma once

#include <volk.h>

class VulkanContext;

namespace render::backend::vulkan
{
	class Platform
	{
	public:
		static const char *getInstanceExtension();
		static VkSurfaceKHR createSurface(const VulkanContext *context, void *native_window);
		static void destroySurface(const VulkanContext *context, VkSurfaceKHR surface);
	};
}
