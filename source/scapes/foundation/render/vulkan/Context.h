#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

namespace scapes::foundation::render::vulkan
{
	/*
	 */
	class Context
	{
	public:
		inline VkInstance getInstance() const { return instance; }
		inline VkDevice getDevice() const { return device; }
		inline VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
		inline VkCommandPool getCommandPool() const { return commandPool; }
		inline VkDescriptorPool getDescriptorPool() const { return descriptorPool; }
		inline uint32_t getGraphicsQueueFamily() const { return graphicsQueueFamily; }
		inline VkQueue getGraphicsQueue() const { return graphicsQueue; }
		inline VkSampleCountFlagBits getMaxSampleCount() const { return maxMSAASamples; }
		inline VmaAllocator getVRAMAllocator() const { return vram_allocator; }

	public:
		void init(const char *applicationName, const char *engineName);
		void shutdown();

	private:
		int examinePhysicalDevice(VkPhysicalDevice physicalDevice) const;

	private:
		enum
		{
			MAX_COMBINED_IMAGE_SAMPLERS = 32,
			MAX_UNIFORM_BUFFERS = 32,
			MAX_DESCRIPTOR_SETS = 512,
		};

		VkInstance instance {VK_NULL_HANDLE};

		VkDevice device {VK_NULL_HANDLE};
		VkPhysicalDevice physicalDevice {VK_NULL_HANDLE};

		VkCommandPool commandPool {VK_NULL_HANDLE};
		VkDescriptorPool descriptorPool {VK_NULL_HANDLE};

		uint32_t graphicsQueueFamily {0xFFFF};
		VkQueue graphicsQueue {VK_NULL_HANDLE};

		VkSampleCountFlagBits maxMSAASamples {VK_SAMPLE_COUNT_1_BIT};
		VkDebugUtilsMessengerEXT debugMessenger {VK_NULL_HANDLE};

		VmaAllocator vram_allocator {VK_NULL_HANDLE};
	};
}
