#pragma once

#include <scapes/Common.h>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace scapes::foundation::render::vulkan
{
	/*
	 */
	class Context
	{
	public:
		SCAPES_INLINE VkInstance getInstance() const { return instance; }
		SCAPES_INLINE VkDevice getDevice() const { return device; }
		SCAPES_INLINE VkPhysicalDevice getPhysicalDevice() const { return physical_device; }
		SCAPES_INLINE VkCommandPool getCommandPool() const { return command_pool; }
		SCAPES_INLINE VkDescriptorPool getDescriptorPool() const { return descriptor_pool; }
		SCAPES_INLINE uint32_t getGraphicsQueueFamily() const { return graphics_queue_family; }
		SCAPES_INLINE VkQueue getGraphicsQueue() const { return graphics_queue; }
		SCAPES_INLINE VkSampleCountFlagBits getMaxSampleCount() const { return max_samples; }
		SCAPES_INLINE VmaAllocator getVRAMAllocator() const { return vram_allocator; }
		SCAPES_INLINE bool hasAccelerationStructure() const { return has_acceleration_structure; }
		SCAPES_INLINE bool hasRayTracing() const { return has_ray_tracing; }
		SCAPES_INLINE bool hasRayQuery() const { return has_ray_query; }

		SCAPES_INLINE uint32_t getSBTHandleAlignment() const { return ray_tracing_properties.shaderGroupHandleAlignment; }
		SCAPES_INLINE uint32_t getSBTHandleSize() const { return ray_tracing_properties.shaderGroupHandleSize; }
		SCAPES_INLINE uint32_t getSBTHandleSizeAligned() const
		{
			const uint32_t size = ray_tracing_properties.shaderGroupHandleSize;
			const uint32_t mask = ray_tracing_properties.shaderGroupHandleAlignment - 1;

			return (size + mask) & ~mask;
		}

		SCAPES_INLINE uint32_t getSBTBaseSizeAligned(uint32_t size) const
		{
			const uint32_t mask = ray_tracing_properties.shaderGroupBaseAlignment - 1;

			return (size + mask) & ~mask;
		}

	public:
		void init(const char *application_name, const char *engine_name);
		void shutdown();

	private:
		int examinePhysicalDevice(VkPhysicalDevice physical_device) const;

	private:
		enum
		{
			MAX_COMBINED_IMAGE_SAMPLERS = 32,
			MAX_UNIFORM_BUFFERS = 32,
			MAX_DESCRIPTOR_SETS = 512,
		};

		bool has_acceleration_structure {false};
		bool has_ray_tracing {false};
		bool has_ray_query {false};

		VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_properties;

		VkInstance instance {VK_NULL_HANDLE};

		VkDevice device {VK_NULL_HANDLE};
		VkPhysicalDevice physical_device {VK_NULL_HANDLE};

		VkCommandPool command_pool {VK_NULL_HANDLE};
		VkDescriptorPool descriptor_pool {VK_NULL_HANDLE};

		uint32_t graphics_queue_family {0xFFFF};
		VkQueue graphics_queue {VK_NULL_HANDLE};

		VkSampleCountFlagBits max_samples {VK_SAMPLE_COUNT_1_BIT};
		VkDebugUtilsMessengerEXT debug_messenger {VK_NULL_HANDLE};

		VmaAllocator vram_allocator {VK_NULL_HANDLE};
	};
}
