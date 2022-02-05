#pragma once

#include <vector>

#include "render/vulkan/Device.h"

namespace scapes::foundation::render::vulkan
{
	class Context;

	/*
	 */
	class Utils
	{
	public:
		static VkTransformMatrixKHR getTransformMatrix(
			const float col_major_matrix[16]
		);

		static VkSamplerAddressMode getSamplerAddressMode(
			SamplerWrapMode mode
		);

		static VkFormat getFormat(
			Format format
		);
		
		static Format getApiFormat(
			VkFormat format
		);
		
		static VkSampleCountFlagBits getSamples(
			Multisample samples
		);
		
		static Multisample getApiSamples(
			VkSampleCountFlagBits samples
		);
		
		static VkImageUsageFlags getImageUsageFlags(
			VkFormat format
		);
		
		static VkImageAspectFlags getImageAspectFlags(
			VkFormat format
		);
		
		static VkImageViewType getImageBaseViewType(
			VkImageType type, VkImageCreateFlags flags, uint32_t num_layers
		);
		
		static uint8_t getPixelSize(
			Format format
		);
		
		static VkIndexType getIndexType(
			IndexFormat format
		);
		
		static uint8_t getIndexSize(
			IndexFormat format
		);

		static uint8_t getVertexSize(
			Format format
		);
		
		static VkPrimitiveTopology getPrimitiveTopology(
			RenderPrimitiveType type
		);
		
		static VkCommandBufferLevel getCommandBufferLevel(
			CommandBufferType type
		);

		static VkCullModeFlags getCullMode(
			CullMode mode
		);

		static VkCompareOp getDepthCompareFunc(
			DepthCompareFunc func
		);

		static VkBlendFactor getBlendFactor(
			BlendFactor factor
		);

		static VkAttachmentLoadOp getLoadOp(
			RenderPassLoadOp op
		);

		static VkAttachmentStoreOp getStoreOp(
			RenderPassStoreOp op
		);

		static bool checkInstanceValidationLayers(
			const std::vector<const char *> &requiredLayers,
			bool verbose = false
		);

		static bool checkInstanceExtensions(
			const std::vector<const char *> &requiredExtensions,
			bool verbose = false
		);

		static bool checkPhysicalDeviceExtensions(
			VkPhysicalDevice physicalDevice,
			const std::vector<const char *> &requiredExtensions,
			bool verbose = false
		);

		static VkSampleCountFlagBits getMaxUsableSampleCount(
			VkPhysicalDevice physicalDevice
		);

		static VkPhysicalDeviceRayTracingPipelinePropertiesKHR getRayTracingPipelineProperties(
			VkPhysicalDevice physicalDevice
		);

		static VkFormat selectOptimalImageFormat(
			VkPhysicalDevice physicalDevice,
			const std::vector<VkFormat> &candidates,
			VkImageTiling tiling,
			VkFormatFeatureFlags features
		);

		static VkFormat selectOptimalDepthFormat(
			VkPhysicalDevice physicalDevice
		);

		static uint32_t getGraphicsQueueFamily(
			VkPhysicalDevice physicalDevice
		);

		static uint32_t getPresentQueueFamily(
			VkPhysicalDevice physicalDevice,
			VkSurfaceKHR surface,
			uint32_t graphicsQueueFamily
		);

		static VkDeviceAddress getBufferDeviceAddress(
			const Context *context,
			VkBuffer
		);

		static void createBuffer(
			const Context *context,
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VmaMemoryUsage memory_usage,
			VkBuffer &buffer,
			VmaAllocation &memory
		);

		static void fillDeviceLocalBuffer(
			const Context *context,
			VkBuffer buffer,
			VkDeviceSize size,
			const void *data
		);

		static void fillHostVisibleBuffer(
			const Context *context,
			VmaAllocation memory,
			VkDeviceSize size,
			const void *data
		);

		static VkShaderModule createShaderModule(
			const Context *context,
			const uint32_t *data,
			size_t size
		);

		static void createImage(
			const Context *context,
			VkImageType type,
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			uint32_t mipLevels,
			uint32_t arrayLayers,
			VkSampleCountFlagBits numSamples,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkImageCreateFlags flags,
			VkImage &image,
			VmaAllocation &memory
		);

		static VkImageView createImageView(
			const Context *context,
			const Texture *texture,
			uint32_t base_mip = 0,
			uint32_t num_mips = 1,
			uint32_t base_layer = 0,
			uint32_t num_layers = 1
		);

		static VkSampler createSampler(
			const Context *context,
			uint32_t minMipLevel,
			uint32_t maxMipLevel,
			VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			bool depthCompareEnabled = false,
			VkCompareOp depthCompareFunc = VK_COMPARE_OP_ALWAYS
		);

		static bool createAccelerationStructure(
			const Context *context,
			VkAccelerationStructureTypeKHR type,
			VkBuildAccelerationStructureFlagBitsKHR build_flags,
			uint32_t num_geometries,
			const VkAccelerationStructureGeometryKHR *geometries,
			const uint32_t *max_primitives,
			AccelerationStructure *result
		);

		static void buildAccelerationStructure(
			const Context *context,
			VkAccelerationStructureTypeKHR type,
			VkBuildAccelerationStructureFlagBitsKHR build_flags,
			uint32_t num_geometries,
			const VkAccelerationStructureGeometryKHR *geometries,
			const VkAccelerationStructureBuildRangeInfoKHR *ranges,
			AccelerationStructure *result
		);

		static void fillImage(
			const Context *context,
			VkImage image,
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			uint32_t mipLevels,
			uint32_t arrayLayers,
			uint32_t pixelSize,
			VkFormat format,
			const void *data,
			uint32_t dataMipLevels,
			uint32_t dataArrayLayers
		);

		static void generateImage2DMipmaps(
			const Context *context,
			VkImage image,
			VkFormat imageFormat,
			uint32_t width,
			uint32_t height,
			uint32_t mipLevels,
			VkFormat format,
			VkFilter filter
		);

		static void transitionImageLayout(
			const Context *context,
			VkImage image,
			VkFormat format,
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			uint32_t baseMipLevel = 0,
			uint32_t numMipLevels = 1,
			uint32_t baseLayer = 0,
			uint32_t numLayers = 1
		);

		static VkCommandBuffer beginSingleTimeCommands(
			const Context *context
		);

		static void endSingleTimeCommands(
			const Context *context,
			VkCommandBuffer commandBuffer
		);
	};
}
