#pragma once

#include <volk.h>
#include <vector>

/*
 */
struct VulkanRendererContext
{
	VkDevice device {VK_NULL_HANDLE};
	VkPhysicalDevice physicalDevice {VK_NULL_HANDLE};
	VkCommandPool commandPool {VK_NULL_HANDLE};
	VkQueue graphicsQueue {VK_NULL_HANDLE};
	VkQueue presentQueue {VK_NULL_HANDLE};
	VkSampleCountFlagBits maxMSAASamples {VK_SAMPLE_COUNT_1_BIT};
};

struct VulkanSwapChainContext
{
	VkDescriptorPool descriptorPool {VK_NULL_HANDLE};
	VkFormat colorFormat;
	VkFormat depthFormat;
	VkExtent2D extent;
	std::vector<VkImageView> swapChainImageViews;
	VkImageView depthImageView {VK_NULL_HANDLE};
	VkImageView colorImageView {VK_NULL_HANDLE};
};
