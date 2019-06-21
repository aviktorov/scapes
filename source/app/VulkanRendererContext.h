#pragma once

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>

/*
 */
struct RendererContext
{
	VkDevice device {VK_NULL_HANDLE};
	VkPhysicalDevice physicalDevice {VK_NULL_HANDLE};
	VkCommandPool commandPool {VK_NULL_HANDLE};
	VkDescriptorPool descriptorPool;
	VkFormat format;
	VkExtent2D extent;
	std::vector<VkImageView> swapChainImageViews;
	VkQueue graphicsQueue {VK_NULL_HANDLE};
	VkQueue presentQueue {VK_NULL_HANDLE};
};
