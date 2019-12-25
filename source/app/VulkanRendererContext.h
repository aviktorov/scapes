#pragma once

#include <volk.h>
#include <vector>

/*
 */
struct VulkanRendererContext
{
	VkInstance instance {VK_NULL_HANDLE};
	VkSurfaceKHR surface {VK_NULL_HANDLE};
	VkDevice device {VK_NULL_HANDLE};
	VkPhysicalDevice physicalDevice {VK_NULL_HANDLE};
	VkCommandPool commandPool {VK_NULL_HANDLE};
	VkDescriptorPool descriptorPool {VK_NULL_HANDLE};
	uint32_t graphicsQueueFamily {0};
	uint32_t presentQueueFamily {0};
	VkQueue graphicsQueue {VK_NULL_HANDLE};
	VkQueue presentQueue {VK_NULL_HANDLE};
	VkSampleCountFlagBits maxMSAASamples {VK_SAMPLE_COUNT_1_BIT};
};

struct VulkanSwapChainContext
{
	VkFormat colorFormat;
	VkFormat depthFormat;
	VkExtent2D extent;
	std::vector<VkImageView> swapChainImageViews;
	VkImageView depthImageView {VK_NULL_HANDLE};
	VkImageView colorImageView {VK_NULL_HANDLE};
};
