#pragma once

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

struct RendererContext;

/*
 */
class VulkanUtils
{
public:
	static uint32_t findMemoryType(
		const RendererContext &context,
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties
	);

	static void createBuffer(
		const RendererContext &context,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags memoryProperties,
		VkBuffer &buffer,
		VkDeviceMemory &memory
	);

	static void createImage2D(
		const RendererContext &context,
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags memoryProperties,
		VkImage &image,
		VkDeviceMemory &memory
	);

	static VkImageView createImage2DView(
		const RendererContext &context,
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags
	);

	static VkSampler createSampler(
		const RendererContext &context
	);

	static void copyBuffer(
		const RendererContext &context,
		VkBuffer src,
		VkBuffer dst,
		VkDeviceSize size
	);

	static void copyBufferToImage(
		const RendererContext &context,
		VkBuffer src,
		VkImage dst,
		uint32_t width,
		uint32_t height
	);

	static void transitionImageLayout(
		const RendererContext &context,
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout
	);

private:
	static bool hasStencilComponent(VkFormat format);
	static VkCommandBuffer beginSingleTimeCommands(const RendererContext &context);
	static void endSingleTimeCommands(const RendererContext &context, VkCommandBuffer commandBuffer);
};
