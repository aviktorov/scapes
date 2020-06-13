#pragma once

#include <volk.h>
#include <optional>

namespace render::backend::vulkan
{
	/*
	 */
	class Device
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

	public:
		void init(const char *applicationName, const char *engineName);
		void shutdown();
		void wait();

	private:
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphicsFamily {std::nullopt};
			std::optional<uint32_t> presentFamily {std::nullopt};

			inline bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
		};

		int examinePhysicalDevice(VkPhysicalDevice physicalDevice) const;

	private:
		VkInstance instance {VK_NULL_HANDLE};

		VkDevice device {VK_NULL_HANDLE};
		VkPhysicalDevice physicalDevice {VK_NULL_HANDLE};

		VkCommandPool commandPool {VK_NULL_HANDLE};
		VkDescriptorPool descriptorPool {VK_NULL_HANDLE};

		uint32_t graphicsQueueFamily {0xFFFF};
		VkQueue graphicsQueue {VK_NULL_HANDLE};

		VkSampleCountFlagBits maxMSAASamples {VK_SAMPLE_COUNT_1_BIT};
		VkDebugUtilsMessengerEXT debugMessenger {VK_NULL_HANDLE};

		// TODO: depth buffer
		// TODO: msaa resolve buffer
	};
}
