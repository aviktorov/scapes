#include "render/vulkan/Context.h"
#include "render/vulkan/Platform.h"
#include "render/vulkan/Utils.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <array>
#include <vector>
#include <iostream>
#include <set>
#include <optional>

// TODO: replace all expection throws by Log::fatal

namespace scapes::foundation::render::vulkan
{
	/*
	 */
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily {std::nullopt};
		std::optional<uint32_t> presentFamily {std::nullopt};

		inline bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
	};

	/*
	 */
	static std::vector<const char*> requiredInstanceExtensions = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
	};

	/*
	 */
	static std::vector<const char*> requiredPhysicalDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

#ifdef SCAPES_VULKAN_USE_VALIDATION_LAYERS
	static std::vector<const char *> requiredValidationLayers = {
		"VK_LAYER_KHRONOS_validation",
	};
#endif

	/*
	 */
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	/*
	 */
	void Context::init(const char *applicationName, const char *engineName)
	{
		if (volkInitialize() != VK_SUCCESS)
			throw std::runtime_error("Can't initialize Vulkan helper library");

		// Check required instance extensions
		requiredInstanceExtensions.push_back(render::vulkan::Platform::getInstanceExtension());
		if (!Utils::checkInstanceExtensions(requiredInstanceExtensions, true))
			throw std::runtime_error("This device doesn't have required Vulkan extensions");

#if SCAPES_VULKAN_USE_VALIDATION_LAYERS
		// Check required instance validation layers
		if (!Utils::checkInstanceValidationLayers(requiredValidationLayers, true))
			throw std::runtime_error("This device doesn't have required Vulkan validation layers");
#endif

		// Fill instance structures
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = applicationName;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = engineName;
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
		instanceInfo.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensions.size());
		instanceInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
		instanceInfo.pNext = &debugMessengerInfo;

#if SCAPES_VULKAN_USE_VALIDATION_LAYERS
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
		instanceInfo.ppEnabledLayerNames = requiredValidationLayers.data();
#endif

		// Create Vulkan instance
		VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to create Vulkan instance");

		volkLoadInstance(instance);

		// Create Vulkan debug messenger
		result = vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, &debugMessenger);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Can't create Vulkan debug messenger");

		// Enumerate physical devices
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0)
			throw std::runtime_error("Failed to find GPUs with Vulkan support");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		// Pick the best physical device
		int estimate = -1;
		for (const auto& device : devices)
		{
			int currentEstimate = examinePhysicalDevice(device);
			if (currentEstimate == -1)
				continue;

			if (estimate > currentEstimate)
				continue;

			estimate = currentEstimate;
			physicalDevice = device;
		}

		if (physicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("Failed to find a suitable GPU");

		// Create logical device
		graphicsQueueFamily = Utils::getGraphicsQueueFamily(physicalDevice);
		const float queuePriority = 1.0f;

		VkDeviceQueueCreateInfo graphicsQueueInfo = {};
		graphicsQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphicsQueueInfo.queueFamilyIndex = graphicsQueueFamily;
		graphicsQueueInfo.queueCount = 1;
		graphicsQueueInfo.pQueuePriorities = &queuePriority;

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.sampleRateShading = VK_TRUE;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &graphicsQueueInfo;
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredPhysicalDeviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = requiredPhysicalDeviceExtensions.data();

		// next two parameters are ignored, but it's still good to pass layers for backward compatibility
#if SCAPES_VULKAN_USE_VALIDATION_LAYERS
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size()); 
		deviceCreateInfo.ppEnabledLayerNames = requiredValidationLayers.data();
#endif

		result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Can't create logical device");

		volkLoadDevice(device);

		// Get graphics queue
		vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
		if (graphicsQueue == VK_NULL_HANDLE)
			throw std::runtime_error("Can't get graphics queue from logical device");

		// Create command pool
		VkCommandPoolCreateInfo commandPoolInfo = {};
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.queueFamilyIndex = graphicsQueueFamily;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
			throw std::runtime_error("Can't create command pool");

		// Create descriptor pools
		std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes = {};
		descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorPoolSizes[0].descriptorCount = MAX_UNIFORM_BUFFERS;
		descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorPoolSizes[1].descriptorCount = MAX_COMBINED_IMAGE_SAMPLERS;

		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
		descriptorPoolInfo.pPoolSizes = descriptorPoolSizes.data();
		descriptorPoolInfo.maxSets = MAX_DESCRIPTOR_SETS;
		descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Can't create descriptor pool");

		maxMSAASamples = Utils::getMaxUsableSampleCount(physicalDevice);

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = device;
		allocatorInfo.instance = instance;

		if (vmaCreateAllocator(&allocatorInfo, &vram_allocator) != VK_SUCCESS)
			throw std::runtime_error("Can't create VRAM allocator");
	}

	void Context::shutdown()
	{
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		descriptorPool = VK_NULL_HANDLE;

		vkDestroyCommandPool(device, commandPool, nullptr);
		commandPool = VK_NULL_HANDLE;

		vmaDestroyAllocator(vram_allocator);
		vram_allocator = VK_NULL_HANDLE;

		vkDestroyDevice(device, nullptr);
		device = VK_NULL_HANDLE;

		vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		debugMessenger = VK_NULL_HANDLE;

		vkDestroyInstance(instance, nullptr);
		instance = VK_NULL_HANDLE;

		graphicsQueueFamily = 0xFFFF;
		graphicsQueue = VK_NULL_HANDLE;

		maxMSAASamples = VK_SAMPLE_COUNT_1_BIT;
		physicalDevice = VK_NULL_HANDLE;
	}

	/*
	 */
	int Context::examinePhysicalDevice(VkPhysicalDevice physicalDevice) const
	{
		if (!Utils::checkPhysicalDeviceExtensions(physicalDevice, requiredPhysicalDeviceExtensions))
			return -1;

		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

		int estimate = 0;

		switch (physicalDeviceProperties.deviceType)
		{
			case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			case VK_PHYSICAL_DEVICE_TYPE_CPU: estimate = 10; break;

			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: estimate = 100; break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: estimate = 1000; break;
		}

		// TODO: add more estimates here
		if (physicalDeviceFeatures.geometryShader)
			estimate++;

		if (physicalDeviceFeatures.tessellationShader)
			estimate++;

		return estimate;
	}
}
