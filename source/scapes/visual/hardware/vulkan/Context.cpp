#include <hardware/vulkan/Context.h>
#include <hardware/vulkan/Platform.h>
#include <hardware/vulkan/Utils.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <array>
#include <vector>
#include <iostream>

// TODO: replace all expection throws by Log::fatal

namespace scapes::visual::hardware::vulkan
{
	/*
	 */
	static std::vector<const char *> required_instance_extensions = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
	};

	/*
	 */
	static std::vector<const char *> required_device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	static std::vector<const char *> acceleration_structure_device_extensions = {
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
	};

	static std::vector<const char *> ray_tracing_device_extensions = {
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
	};

	static std::vector<const char *> ray_query_device_extensions = {
		VK_KHR_RAY_QUERY_EXTENSION_NAME,
	};

	/*
	 */
#ifdef SCAPES_VULKAN_USE_VALIDATION_LAYERS
	static std::vector<const char *> required_validation_layers = {
		"VK_LAYER_KHRONOS_validation",
	};
#endif

	/*
	 */
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
		void *user_data)
	{
		std::cerr << "Validation layer: " << callback_data->pMessage << std::endl;
		return VK_FALSE;
	}

	/*
	 */
	void Context::init(const char *application_name, const char *engine_name)
	{
		if (volkInitialize() != VK_SUCCESS)
			throw std::runtime_error("Can't initialize Vulkan helper library");

		// Check required instance extensions
		required_instance_extensions.push_back(Platform::getInstanceExtension());
		if (!Utils::checkInstanceExtensions(required_instance_extensions, true))
			throw std::runtime_error("This device doesn't have required Vulkan extensions");

#if SCAPES_VULKAN_USE_VALIDATION_LAYERS
		// Check required instance validation layers
		if (!Utils::checkInstanceValidationLayers(required_validation_layers, true))
			throw std::runtime_error("This device doesn't have required Vulkan validation layers");
#endif

		// Fill instance structures
		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = application_name;
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = engine_name;
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_2;

		VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info = {};
		debug_messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debug_messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debug_messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debug_messenger_info.pfnUserCallback = debugCallback;
		debug_messenger_info.pUserData = nullptr;

		VkInstanceCreateInfo instance_info = {};
		instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_info.pApplicationInfo = &app_info;
		instance_info.enabledExtensionCount = static_cast<uint32_t>(required_instance_extensions.size());
		instance_info.ppEnabledExtensionNames = required_instance_extensions.data();
		instance_info.pNext = &debug_messenger_info;

#if SCAPES_VULKAN_USE_VALIDATION_LAYERS
		instance_info.enabledLayerCount = static_cast<uint32_t>(required_validation_layers.size());
		instance_info.ppEnabledLayerNames = required_validation_layers.data();
#endif

		// Create Vulkan instance
		VkResult result = vkCreateInstance(&instance_info, nullptr, &instance);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to create Vulkan instance");

		volkLoadInstance(instance);

		// Create Vulkan debug messenger
		result = vkCreateDebugUtilsMessengerEXT(instance, &debug_messenger_info, nullptr, &debug_messenger);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Can't create Vulkan debug messenger");

		// Enumerate physical devices
		uint32_t num_devices = 0;
		vkEnumeratePhysicalDevices(instance, &num_devices, nullptr);

		if (num_devices == 0)
			throw std::runtime_error("Failed to find GPUs with Vulkan support");

		std::vector<VkPhysicalDevice> devices(num_devices);
		vkEnumeratePhysicalDevices(instance, &num_devices, devices.data());

		// Pick the best physical device
		int estimate = -1;
		for (const auto& device : devices)
		{
			int current_estimate = examinePhysicalDevice(device);
			if (current_estimate == -1)
				continue;

			if (estimate > current_estimate)
				continue;

			estimate = current_estimate;
			physical_device = device;
		}

		if (physical_device == VK_NULL_HANDLE)
			throw std::runtime_error("Failed to find a suitable GPU");

		// Create logical device
		std::vector<const char *> device_extensions;
		device_extensions.insert(device_extensions.begin(), required_device_extensions.begin(), required_device_extensions.end());

		if (Utils::checkPhysicalDeviceExtensions(physical_device, acceleration_structure_device_extensions))
		{
			device_extensions.insert(
				device_extensions.begin(),
				acceleration_structure_device_extensions.begin(),
				acceleration_structure_device_extensions.end()
			);
			has_acceleration_structure = true;
		}

		if (Utils::checkPhysicalDeviceExtensions(physical_device, ray_tracing_device_extensions))
		{
			device_extensions.insert(
				device_extensions.begin(),
				ray_tracing_device_extensions.begin(),
				ray_tracing_device_extensions.end()
			);
			has_ray_tracing = true;
		}

		if (Utils::checkPhysicalDeviceExtensions(physical_device, ray_query_device_extensions))
		{
			device_extensions.insert(
				device_extensions.begin(),
				ray_query_device_extensions.begin(),
				ray_query_device_extensions.end()
			);
			has_ray_query = true;
		}

		graphics_queue_family = Utils::getGraphicsQueueFamily(physical_device);
		const float queue_priority = 1.0f;

		VkDeviceQueueCreateInfo graphics_queue_info = {};
		graphics_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphics_queue_info.queueFamilyIndex = graphics_queue_family;
		graphics_queue_info.queueCount = 1;
		graphics_queue_info.pQueuePriorities = &queue_priority;

		VkPhysicalDeviceFeatures2 device_features = {};
		device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		device_features.features.samplerAnisotropy = VK_TRUE;
		device_features.features.sampleRateShading = VK_TRUE;
		device_features.features.geometryShader = VK_TRUE;
		device_features.features.tessellationShader = VK_TRUE;

		VkDeviceCreateInfo device_info = {};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.queueCreateInfoCount = 1;
		device_info.pQueueCreateInfos = &graphics_queue_info;
		device_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
		device_info.ppEnabledExtensionNames = device_extensions.data();
		device_info.pNext = &device_features;

		VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_info = {};
		buffer_device_address_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		buffer_device_address_info.bufferDeviceAddress = VK_TRUE;
		
		VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_info = {};
		acceleration_structure_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		acceleration_structure_info.accelerationStructure = has_acceleration_structure;

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracing_pipeline_info = {};
		raytracing_pipeline_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		raytracing_pipeline_info.rayTracingPipeline = has_ray_tracing;

		VkPhysicalDeviceRayQueryFeaturesKHR rayquery_info = {};
		rayquery_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
		rayquery_info.rayQuery = has_ray_query;

		device_features.pNext = &buffer_device_address_info;
		buffer_device_address_info.pNext = &acceleration_structure_info;

		if (has_ray_tracing)
			acceleration_structure_info.pNext = &raytracing_pipeline_info;

		if (has_ray_query)
			raytracing_pipeline_info.pNext = &rayquery_info;

		// next two parameters are ignored, but it's still good to pass layers for backward compatibility
#if SCAPES_VULKAN_USE_VALIDATION_LAYERS
		device_info.enabledLayerCount = static_cast<uint32_t>(required_validation_layers.size()); 
		device_info.ppEnabledLayerNames = required_validation_layers.data();
#endif

		result = vkCreateDevice(physical_device, &device_info, nullptr, &device);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Can't create logical device");

		volkLoadDevice(device);

		// Get graphics queue
		vkGetDeviceQueue(device, graphics_queue_family, 0, &graphics_queue);
		if (graphics_queue == VK_NULL_HANDLE)
			throw std::runtime_error("Can't get graphics queue from logical device");

		// Create command pool
		VkCommandPoolCreateInfo command_pool_info = {};
		command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		command_pool_info.queueFamilyIndex = graphics_queue_family;
		command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device, &command_pool_info, nullptr, &command_pool) != VK_SUCCESS)
			throw std::runtime_error("Can't create command pool");

		// Create descriptor pools
		std::array<VkDescriptorPoolSize, 3> descriptor_pool_sizes = {};
		descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_pool_sizes[0].descriptorCount = MAX_UNIFORM_BUFFERS;
		descriptor_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_pool_sizes[1].descriptorCount = MAX_COMBINED_IMAGE_SAMPLERS;
		descriptor_pool_sizes[2].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		descriptor_pool_sizes[2].descriptorCount = MAX_ACCELERATION_STRUCTURES;

		VkDescriptorPoolCreateInfo descriptor_pool_info = {};
		descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptor_pool_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
		descriptor_pool_info.pPoolSizes = descriptor_pool_sizes.data();
		descriptor_pool_info.maxSets = MAX_DESCRIPTOR_SETS;
		descriptor_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(device, &descriptor_pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
			throw std::runtime_error("Can't create descriptor pool");

		max_samples = Utils::getMaxUsableSampleCount(physical_device);
		ray_tracing_properties = Utils::getRayTracingPipelineProperties(physical_device);

		VmaAllocatorCreateInfo allocator_info = {};
		allocator_info.physicalDevice = physical_device;
		allocator_info.device = device;
		allocator_info.instance = instance;
		allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		if (vmaCreateAllocator(&allocator_info, &vram_allocator) != VK_SUCCESS)
			throw std::runtime_error("Can't create VRAM allocator");
	}

	void Context::shutdown()
	{
		vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
		descriptor_pool = VK_NULL_HANDLE;

		vkDestroyCommandPool(device, command_pool, nullptr);
		command_pool = VK_NULL_HANDLE;

		vmaDestroyAllocator(vram_allocator);
		vram_allocator = VK_NULL_HANDLE;

		vkDestroyDevice(device, nullptr);
		device = VK_NULL_HANDLE;

		vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
		debug_messenger = VK_NULL_HANDLE;

		vkDestroyInstance(instance, nullptr);
		instance = VK_NULL_HANDLE;

		graphics_queue_family = 0xFFFF;
		graphics_queue = VK_NULL_HANDLE;

		max_samples = VK_SAMPLE_COUNT_1_BIT;
		physical_device = VK_NULL_HANDLE;
	}

	/*
	 */
	int Context::examinePhysicalDevice(VkPhysicalDevice physical_device) const
	{
		if (!Utils::checkPhysicalDeviceExtensions(physical_device, required_device_extensions, true))
			return -1;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physical_device, &properties);

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(physical_device, &features);

		int estimate = 0;

		switch (properties.deviceType)
		{
			case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			case VK_PHYSICAL_DEVICE_TYPE_CPU: estimate = 10; break;

			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: estimate = 100; break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: estimate = 1000; break;
		}

		if (!features.geometryShader)
			return -1;

		if (!features.tessellationShader)
			return -1;

		if (Utils::checkPhysicalDeviceExtensions(physical_device, acceleration_structure_device_extensions))
			estimate += 1000;

		if (Utils::checkPhysicalDeviceExtensions(physical_device, ray_tracing_device_extensions))
			estimate += 1000;

		if (Utils::checkPhysicalDeviceExtensions(physical_device, ray_query_device_extensions))
			estimate += 1000;

		// TODO: add more estimates here

		return estimate;
	}
}
