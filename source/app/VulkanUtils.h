#pragma once

#include <volk.h>

struct VulkanRendererContext;

/*
 */
class VulkanUtils
{
public:
	static uint32_t findMemoryType(
		const VulkanRendererContext &context,
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties
	);

	static void createBuffer(
		const VulkanRendererContext &context,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags memoryProperties,
		VkBuffer &buffer,
		VkDeviceMemory &memory
	);

	static void createImage2D(
		const VulkanRendererContext &context,
		uint32_t width,
		uint32_t height,
		uint32_t mipLevels,
		VkSampleCountFlagBits numSamples,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags memoryProperties,
		VkImage &image,
		VkDeviceMemory &memory
	);

	static VkImageView createImage2DView(
		const VulkanRendererContext &context,
		VkImage image,
		uint32_t mipLevels,
		VkFormat format,
		VkImageAspectFlags aspectFlags
	);

	static VkSampler createSampler(
		const VulkanRendererContext &context,
		uint32_t mipLevels
	);

	static void copyBuffer(
		const VulkanRendererContext &context,
		VkBuffer src,
		VkBuffer dst,
		VkDeviceSize size
	);

	static void copyBufferToImage(
		const VulkanRendererContext &context,
		VkBuffer src,
		VkImage dst,
		uint32_t width,
		uint32_t height
	);

	static void transitionImageLayout(
		const VulkanRendererContext &context,
		VkImage image,
		uint32_t mipLevels,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout
	);

	static void generateImage2DMipmaps(
		const VulkanRendererContext &context,
		VkImage image,
		uint32_t width,
		uint32_t height,
		uint32_t mipLevels,
		VkFormat format,
		VkFilter filter
	);

	static VkSampleCountFlagBits getMaxUsableSampleCount(
		const VulkanRendererContext &context
	);

private:
	static bool hasStencilComponent(VkFormat format);
	static VkCommandBuffer beginSingleTimeCommands(const VulkanRendererContext &context);
	static void endSingleTimeCommands(const VulkanRendererContext &context, VkCommandBuffer commandBuffer);
};
