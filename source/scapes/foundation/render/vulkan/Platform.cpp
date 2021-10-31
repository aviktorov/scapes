#include <iostream>

#include "render/vulkan/Context.h"
#include "render/vulkan/Platform.h"

namespace scapes::foundation::render::vulkan
{
	const char *Platform::getInstanceExtension()
	{
		#if defined(SCAPES_PLATFORM_WIN32)
			return "VK_KHR_win32_surface";
		#else
			#error "Platform::getInstanceExtension(): not supported on this platform"
		#endif
	}

	VkSurfaceKHR Platform::createSurface(const Context *context, void *native_window)
	{
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		#if defined(SCAPES_PLATFORM_WIN32)
			VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
			surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			surfaceInfo.hwnd = reinterpret_cast<HWND>(native_window);
			surfaceInfo.hinstance = GetModuleHandle(nullptr);

			if (vkCreateWin32SurfaceKHR(context->getInstance(), &surfaceInfo, nullptr, &surface) != VK_SUCCESS)
				std::cerr << "Platform::createSurface(): vkCreateWin32SurfaceKHR failed" << std::endl;

		#else
			std::cerr << "Platform::createSurface(): not supported on this platform" << std::endl;
		#endif

		return surface;
	}

	void Platform::destroySurface(const Context *context, VkSurfaceKHR surface)
	{
		vkDestroySurfaceKHR(context->getInstance(), surface, nullptr);
	}
}
