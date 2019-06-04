#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <GLM/vec4.hpp>

#include <vector>
#include <iostream>
#include <stdexcept>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

struct QueueFamilyIndices {
	std::pair<bool, uint32_t> graphicsFamily;

	bool isComplete() {
		return graphicsFamily.first;
	}
};

class Application
{
public:
	void run();

private:
	void initWindow();
	void shutdownWindow();

	bool checkRequiredValidationLayers(std::vector<const char *> &layers) const;
	bool checkRequiredExtensions(std::vector<const char *> &extensions) const;
	bool checkPhysicalDevice(VkPhysicalDevice device) const;

	QueueFamilyIndices fetchQueueFamilyIndices(VkPhysicalDevice device) const;

	void initVulkanExtensions();

	void initVulkan();
	void shutdownVulkan();

	void mainloop();

private:
	GLFWwindow *window {nullptr};

	VkInstance instance {VK_NULL_HANDLE};
	VkDebugUtilsMessengerEXT debugMessenger {VK_NULL_HANDLE};
	VkPhysicalDevice physicalDevice {VK_NULL_HANDLE};
	VkDevice device {VK_NULL_HANDLE};
	VkQueue graphicsQueue {VK_NULL_HANDLE};

	static PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugMessenger;
	static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugMessenger;
};

PFN_vkCreateDebugUtilsMessengerEXT Application::vkCreateDebugMessenger {VK_NULL_HANDLE};
PFN_vkDestroyDebugUtilsMessengerEXT Application::vkDestroyDebugMessenger {VK_NULL_HANDLE};

void Application::run()
{
	initWindow();
	initVulkan();
	mainloop();
	shutdownVulkan();
	shutdownWindow();
}

void Application::initWindow()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	const int width = 640;
	const int height = 480;

	window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
}

bool Application::checkRequiredValidationLayers(std::vector<const char *> &layers) const
{
	uint32_t vulkanLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&vulkanLayerCount, nullptr);
	std::vector<VkLayerProperties> vulkanLayers(vulkanLayerCount);
	vkEnumerateInstanceLayerProperties(&vulkanLayerCount, vulkanLayers.data());

	static std::vector<const char *> requiredLayers = {
		"VK_LAYER_KHRONOS_validation",
	};

	layers.clear();
	for (const auto &requiredLayer : requiredLayers)
	{
		bool supported = false;
		for (const auto &vulkanLayer : vulkanLayers)
		{
			if (strcmp(requiredLayer, vulkanLayer.layerName) == 0)
			{
				supported = true;
				break;
			}
		}

		if (!supported)
		{
			std::cerr << requiredLayer << " is not supported on this device" << std::endl;
			return false;
		}
		
		std::cout << "Have " << requiredLayer << std::endl;
		layers.push_back(requiredLayer);
	}

	return true;
}

bool Application::checkRequiredExtensions(std::vector<const char *> &extensions) const
{
	uint32_t vulkanExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &vulkanExtensionCount, nullptr);
	std::vector<VkExtensionProperties> vulkanExtensions(vulkanExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &vulkanExtensionCount, vulkanExtensions.data());

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char *> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	extensions.clear();
	for (const auto &requiredExtension : requiredExtensions)
	{
		bool supported = false;
		for (const auto &vulkanExtension : vulkanExtensions)
		{
			if (strcmp(requiredExtension, vulkanExtension.extensionName) == 0)
			{
				supported = true;
				break;
			}
		}

		if (!supported)
		{
			std::cerr << requiredExtension << " is not supported on this device" << std::endl;
			return false;
		}
		
		std::cout << "Have " << requiredExtension << std::endl;
		extensions.push_back(requiredExtension);
	}

	return true;
}

bool Application::checkPhysicalDevice(VkPhysicalDevice device) const
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	QueueFamilyIndices indices = fetchQueueFamilyIndices(device);
	if (!indices.isComplete())
		return false;

	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;
}

QueueFamilyIndices Application::fetchQueueFamilyIndices(VkPhysicalDevice device) const
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	QueueFamilyIndices indices = {};

	for (const auto &queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = std::make_pair(true, i);
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

void Application::initVulkan()
{
	// Check required extensions & layers
	std::vector<const char *> extensions;
	if (!checkRequiredExtensions(extensions))
		throw std::runtime_error("This device doesn't have required Vulkan extensions");

	std::vector<const char *> layers;
	if (!checkRequiredValidationLayers(layers))
		throw std::runtime_error("This device doesn't have required Vulkan validation layers");

	// Fill instance structures
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "PBR Sandbox";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
	debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugMessengerInfo.pfnUserCallback = debugCallback;
	debugMessengerInfo.pUserData = nullptr;

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceInfo.ppEnabledExtensionNames = extensions.data();
	instanceInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
	instanceInfo.ppEnabledLayerNames = layers.data();
	instanceInfo.pNext = &debugMessengerInfo;

	// Create Vulkan instance
	VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create Vulkan instance");

	initVulkanExtensions();

	// Create Vulkan debug messenger
	result = vkCreateDebugMessenger(instance, &debugMessengerInfo, nullptr, &debugMessenger);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Can't create Vulkan debug messenger");

	// Enumerate physical devices
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		throw std::runtime_error("Failed to find GPUs with Vulkan support");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (checkPhysicalDevice(device))
		{
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to find a suitable GPU");

	// TODO: scores

	// Fill logical device structures
	QueueFamilyIndices indices = fetchQueueFamilyIndices(physicalDevice);

	const float queuePriorities[] = { 1.0f };

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.second;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = queuePriorities;

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledExtensionCount = 0;
	// next two parameters are ignored, but it's still good to pass layers for backward compatibility
	deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size()); 
	deviceCreateInfo.ppEnabledLayerNames = layers.data();

	// Create logical device
	result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Can't create logical device");

	vkGetDeviceQueue(device, indices.graphicsFamily.second, 0, &graphicsQueue);
	if (graphicsQueue == VK_NULL_HANDLE)
		throw std::runtime_error("Can't get graphics queue from logical device");
}

void Application::initVulkanExtensions()
{
	vkCreateDebugMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (!vkCreateDebugMessenger)
		throw std::runtime_error("Can't find vkCreateDebugUtilsMessengerEXT function");

	vkDestroyDebugMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (!vkDestroyDebugMessenger)
		throw std::runtime_error("Can't find vkDestroyDebugUtilsMessengerEXT function");
}

void Application::shutdownWindow()
{
	glfwDestroyWindow(window);
	window = nullptr;
}

void Application::shutdownVulkan()
{
	vkDestroyDevice(device, nullptr);
	device = nullptr;

	vkDestroyDebugMessenger(instance, debugMessenger, nullptr);
	debugMessenger = nullptr;

	vkDestroyInstance(instance, nullptr);
	instance = nullptr;
}

void Application::mainloop()
{
	if (!window)
		return;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}

int main(void)
{
	if (!glfwInit())
		return EXIT_FAILURE;

	try
	{
		Application app;
		app.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwTerminate();
	return EXIT_SUCCESS;
}
