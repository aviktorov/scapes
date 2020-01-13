#pragma once

#include <volk.h>
#include <vector>
#include <optional>

struct GLFWwindow;

/*
 */
class VulkanContext
{
public:
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

public:
	void init(GLFWwindow *window, const char *applicationName, const char *engineName);
	void shutdown();

private:
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily {std::nullopt};
		std::optional<uint32_t> presentFamily {std::nullopt};

		inline bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
	};

	int examinePhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface) const;
	VulkanContext::QueueFamilyIndices fetchQueueFamilyIndices(VkPhysicalDevice device) const;

private:
	VkDebugUtilsMessengerEXT debugMessenger {VK_NULL_HANDLE};
};
