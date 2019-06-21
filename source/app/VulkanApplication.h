#pragma once

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>

struct GLFWwindow;
class Renderer;

/*
 */
struct QueueFamilyIndices
{
	std::pair<bool, uint32_t> graphicsFamily { std::make_pair(false, 0) };
	std::pair<bool, uint32_t> presentFamily { std::make_pair(false, 0) };

	bool isComplete() {
		return graphicsFamily.first && presentFamily.first;
	}
};

/*
 */
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

/*
 */
struct SwapChainSettings
{
	VkSurfaceFormatKHR format;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
};

/*
 */
class Application
{
public:
	void run();

private:
	void initWindow();
	void shutdownWindow();

	bool checkRequiredValidationLayers(std::vector<const char *> &layers) const;
	bool checkRequiredExtensions(std::vector<const char *> &extensions) const;
	bool checkRequiredPhysicalDeviceExtensions(VkPhysicalDevice device, std::vector<const char *> &extensions) const;
	bool checkPhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface) const;

	SwapChainSupportDetails fetchSwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface) const;
	QueueFamilyIndices fetchQueueFamilyIndices(VkPhysicalDevice device) const;

	SwapChainSettings selectOptimalSwapChainSettings(const SwapChainSupportDetails &details) const;

	void initVulkanExtensions();

	void initVulkan();
	void shutdownVulkan();

	void initRenderer();
	void shutdownRenderer();

	void render();
	void mainloop();

private:
	GLFWwindow *window {nullptr};
	Renderer *renderer {nullptr};

	VkInstance instance {VK_NULL_HANDLE};
	VkDebugUtilsMessengerEXT debugMessenger {VK_NULL_HANDLE};
	VkPhysicalDevice physicalDevice {VK_NULL_HANDLE};
	VkDevice device {VK_NULL_HANDLE};
	VkQueue graphicsQueue {VK_NULL_HANDLE};
	VkQueue presentQueue {VK_NULL_HANDLE};
	VkSurfaceKHR surface {VK_NULL_HANDLE};

	VkSwapchainKHR swapChain {VK_NULL_HANDLE};
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;

	VkImage depthImage;
	VkImageView depthImageView;
	VkDeviceMemory depthImageMemory;

	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	VkDescriptorPool descriptorPool;
	VkCommandPool commandPool {VK_NULL_HANDLE};

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	size_t currentFrame {0};

	enum
	{
		WIDTH = 640,
		HEIGHT = 480,
	};

	enum
	{
		MAX_FRAMES_IN_FLIGHT = 2,
	};
};
