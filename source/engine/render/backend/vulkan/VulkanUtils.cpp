#include "render/backend/vulkan/VulkanUtils.h"
#include "render/backend/vulkan/device.h"

#include <algorithm>
#include <iostream>

/*
 */
bool VulkanUtils::checkInstanceValidationLayers(
	const std::vector<const char *> &requiredLayers,
	bool verbose
)
{
	uint32_t availableLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(availableLayerCount);
	vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

	for (const char *requiredLayer : requiredLayers)
	{
		bool supported = false;
		for (const VkLayerProperties &layer : availableLayers)
		{
			if (strcmp(requiredLayer, layer.layerName) == 0)
			{
				supported = true;
				break;
			}
		}

		if (!supported)
		{
			if (verbose)
				std::cerr << requiredLayer << " is not supported" << std::endl;

			return false;
		}
		
		if (verbose)
			std::cout << "Have " << requiredLayer << std::endl;
	}

	return true;
}

bool VulkanUtils::checkInstanceExtensions(
	const std::vector<const char *> &requiredExtensions,
	bool verbose
)
{
	uint32_t availableExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

	for (const char *requiredExtension : requiredExtensions)
	{
		bool supported = false;
		for (const VkExtensionProperties &availableExtension : availableExtensions)
		{
			if (strcmp(requiredExtension, availableExtension.extensionName) == 0)
			{
				supported = true;
				break;
			}
		}

		if (!supported)
		{
			if (verbose)
				std::cerr << requiredExtension << " is not supported" << std::endl;

			return false;
		}
		
		if (verbose)
			std::cout << "Have " << requiredExtension << std::endl;
	}

	return true;
}

bool VulkanUtils::checkPhysicalDeviceExtensions(
	VkPhysicalDevice physicalDevice,
	const std::vector<const char *> &requiredExtensions,
	bool verbose
)
{
	uint32_t availableDeviceExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableDeviceExtensionCount, nullptr);

	std::vector<VkExtensionProperties> availableDeviceExtensions(availableDeviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableDeviceExtensionCount, availableDeviceExtensions.data());

	for (const char *requiredExtension : requiredExtensions)
	{
		bool supported = false;
		for (const VkExtensionProperties &availableDeviceExtension : availableDeviceExtensions)
		{
			if (strcmp(requiredExtension, availableDeviceExtension.extensionName) == 0)
			{
				supported = true;
				break;
			}
		}

		if (!supported)
		{
			if (verbose)
				std::cerr << requiredExtension << " is not supported on this physical device" << std::endl;

			return false;
		}
		
		if (verbose)
			std::cout << "Have " << requiredExtension << std::endl;
	}

	return true;
}

/*
 */
VkSampleCountFlagBits VulkanUtils::getMaxUsableSampleCount(
	VkPhysicalDevice physicalDevice
)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	// TODO: this might return wrong value in case color & depth have different multisampling options
	VkSampleCountFlags counts = std::min(
		physicalDeviceProperties.limits.framebufferColorSampleCounts,
		physicalDeviceProperties.limits.framebufferDepthSampleCounts
	);

	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

/*
 */
VkFormat VulkanUtils::selectOptimalImageFormat(
	VkPhysicalDevice physicalDevice,
	const std::vector<VkFormat> &candidates,
	VkImageTiling tiling,
	VkFormatFeatureFlags features
)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
			return format;
		
		if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
			return format;
	}

	return VK_FORMAT_UNDEFINED;
}

VkFormat VulkanUtils::selectOptimalDepthFormat(
	VkPhysicalDevice physicalDevice
)
{
	return selectOptimalImageFormat(
		physicalDevice,
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

/*
 */
uint32_t VulkanUtils::findMemoryType(
	VkPhysicalDevice physicalDevice,
	uint32_t typeFilter,
	VkMemoryPropertyFlags properties
)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		uint32_t memoryTypeProperties = memProperties.memoryTypes[i].propertyFlags;
		if ((typeFilter & (1 << i)) && (memoryTypeProperties & properties) == properties)
			return i;
	}

	throw std::runtime_error("Can't find suitable memory type");
}

/*
 */
uint32_t VulkanUtils::getGraphicsQueueFamily(
	VkPhysicalDevice physicalDevice
)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		const auto &queueFamily = queueFamilies[i];
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			return i;
	}

	return 0xFFFF;
}

/*
 */
uint32_t VulkanUtils::getPresentQueueFamily(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface,
	uint32_t graphicsQueueFamily
)
{
	VkBool32 presentSupported = VK_FALSE;

	if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsQueueFamily, surface, &presentSupported) != VK_SUCCESS)
		throw std::runtime_error("Can't check surface present support");

	if (presentSupported)
		return graphicsQueueFamily;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupported) != VK_SUCCESS)
			throw std::runtime_error("Can't check surface present support");

		if (presentSupported)
			return i;
	}

	return 0xFFFF;
}

/*
 */
void VulkanUtils::createBuffer(
	const render::backend::vulkan::Device *device,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags memoryProperties,
	VkBuffer &buffer,
	VkDeviceMemory &memory
)
{
	// Create buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device->getDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("Can't create buffer");

	// Allocate memory for the buffer
	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(device->getDevice(), buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(device->getPhysicalDevice(), memoryRequirements.memoryTypeBits, memoryProperties);

	if (vkAllocateMemory(device->getDevice(), &memoryAllocateInfo, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error("Can't allocate buffer memory");

	if (vkBindBufferMemory(device->getDevice(), buffer, memory, 0) != VK_SUCCESS)
		throw std::runtime_error("Can't bind buffer memory");
}

void VulkanUtils::createDeviceLocalBuffer(
	const render::backend::vulkan::Device *device,
	VkDeviceSize size,
	const void *data,
	VkBufferUsageFlags usage,
	VkBuffer &buffer,
	VkDeviceMemory &memory
)
{
	// Create vertex buffer
	createBuffer(
		device,
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		buffer,
		memory
	);

	// Create staging buffer
	VkBuffer staging_buffer = VK_NULL_HANDLE;
	VkDeviceMemory staging_memory = VK_NULL_HANDLE;

	VulkanUtils::createBuffer(
		device,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer,
		staging_memory
	);

	// Fill staging buffer
	void *staging_data = nullptr;
	vkMapMemory(device->getDevice(), staging_memory, 0, size, 0, &staging_data);
	memcpy(staging_data, data, static_cast<size_t>(size));
	vkUnmapMemory(device->getDevice(), staging_memory);

	// Transfer to GPU local memory
	copyBuffer(device, staging_buffer, buffer, size);

	// Destroy staging buffer
	vkDestroyBuffer(device->getDevice(), staging_buffer, nullptr);
	vkFreeMemory(device->getDevice(), staging_memory, nullptr);
}

/*
 */
void VulkanUtils::createImage(
	const render::backend::vulkan::Device *device,
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
	VkMemoryPropertyFlags memoryProperties,
	VkImageCreateFlags flags,
	VkImage &image,
	VkDeviceMemory &memory
)
{
	// Create buffer
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = type;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = depth;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = arrayLayers;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.usage = usage;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = numSamples;
	imageInfo.flags = flags;

	if (vkCreateImage(device->getDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("Can't create image");

	// Allocate memory for the buffer
	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(device->getDevice(), image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(device->getPhysicalDevice(), memoryRequirements.memoryTypeBits, memoryProperties);

	if (vkAllocateMemory(device->getDevice(), &memoryAllocateInfo, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error("Can't allocate image memory");

	if (vkBindImageMemory(device->getDevice(), image, memory, 0) != VK_SUCCESS)
		throw std::runtime_error("Can't bind image memory");
}

/*
 */
void VulkanUtils::createImageCube(
	const render::backend::vulkan::Device *device,
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
)
{
	createImage(
		device,
		VK_IMAGE_TYPE_2D,
		width, height, 1,
		mipLevels, 6, numSamples,
		format, tiling,
		usage, memoryProperties,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
		image, memory
	);
}

void VulkanUtils::createImage2D(
	const render::backend::vulkan::Device *device,
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
)
{
	createImage(
		device,
		VK_IMAGE_TYPE_2D,
		width, height, 1,
		mipLevels, 1, numSamples,
		format, tiling,
		usage, memoryProperties,
		0,
		image, memory
	);
}

void VulkanUtils::fillImage(
	const render::backend::vulkan::Device *device,
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
)
{
	VkDeviceSize resource_size = 0;
	uint32_t mip_width = width;
	uint32_t mip_height = height;
	uint32_t mip_depth = depth;

	for (uint32_t i = 0; i < dataMipLevels; i++)
	{
		resource_size += mip_width * mip_height * pixelSize;
		mip_width = std::max<int>(mip_width / 2, 1);
		mip_height = std::max<int>(mip_height / 2, 1);
		mip_depth = std::max<int>(mip_depth / 2, 1);
	}

	// Create staging buffer
	VkBuffer staging_buffer = VK_NULL_HANDLE;
	VkDeviceMemory staging_memory = VK_NULL_HANDLE;

	createBuffer(
		device,
		resource_size * dataArrayLayers,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer,
		staging_memory
	);

	// Fill staging buffer
	void *staging_data = nullptr;
	vkMapMemory(device->getDevice(), staging_memory, 0, resource_size * dataArrayLayers, 0, &staging_data);
	memcpy(staging_data, data, static_cast<size_t>(resource_size * dataArrayLayers));
	vkUnmapMemory(device->getDevice(), staging_memory);

	// Copy to the image memory on GPU
	VkDeviceSize offset = 0;

	VkCommandBuffer command_buffer = beginSingleTimeCommands(device);

	for (uint32_t i = 0; i < dataArrayLayers; i++)
	{
		mip_width = width;
		mip_height = height;
		mip_depth = depth;

		for (uint32_t j = 0; j < dataMipLevels; j++)
		{
			VkBufferImageCopy region = {};
			region.bufferOffset = offset;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = j;
			region.imageSubresource.baseArrayLayer = i;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = {0, 0, 0};
			region.imageExtent.width = width;
			region.imageExtent.height = height;
			region.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(command_buffer, staging_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			offset += mip_width * mip_height * mip_depth * pixelSize;
			mip_width /= 2;
			mip_height /= 2;
		}
	}

	VulkanUtils::endSingleTimeCommands(device, command_buffer);

	// Destroy staging buffer
	vkDestroyBuffer(device->getDevice(), staging_buffer, nullptr);
	vkFreeMemory(device->getDevice(), staging_memory, nullptr);
}

VkImageView VulkanUtils::createImageView(
	const render::backend::vulkan::Device *device,
	VkImage image,
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	VkImageViewType viewType,
	uint32_t baseMipLevel,
	uint32_t numMipLevels,
	uint32_t baseLayer,
	uint32_t numLayers
)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = viewType;
	viewInfo.format = format;
	viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = baseMipLevel;
	viewInfo.subresourceRange.levelCount = numMipLevels;
	viewInfo.subresourceRange.baseArrayLayer = baseLayer;
	viewInfo.subresourceRange.layerCount = numLayers;

	VkImageView imageView = VK_NULL_HANDLE;
	if (vkCreateImageView(device->getDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		return VK_NULL_HANDLE;

	return imageView;
}

VkSampler VulkanUtils::createSampler(
	const render::backend::vulkan::Device *device,
	uint32_t minMipLevel,
	uint32_t maxMipLevel
)
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = static_cast<float>(minMipLevel);
	samplerInfo.maxLod = static_cast<float>(maxMipLevel);

	VkSampler sampler = VK_NULL_HANDLE;
	if (vkCreateSampler(device->getDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
		return VK_NULL_HANDLE;

	return sampler;
}

VkShaderModule VulkanUtils::createShaderModule(
	const render::backend::vulkan::Device *device,
	const uint32_t *bytecode,
	size_t bytecodeSize
)
{
	VkShaderModuleCreateInfo shaderInfo = {};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.codeSize = bytecodeSize;
	shaderInfo.pCode = bytecode;

	VkShaderModule shader = VK_NULL_HANDLE;
	if (vkCreateShaderModule(device->getDevice(), &shaderInfo, nullptr, &shader) != VK_SUCCESS)
		return VK_NULL_HANDLE;

	return shader;
}

void VulkanUtils::bindUniformBuffer(
	const render::backend::vulkan::Device *device,
	VkDescriptorSet descriptorSet,
	int binding,
	VkBuffer buffer,
	VkDeviceSize offset,
	VkDeviceSize size
)
{
	VkDescriptorBufferInfo descriptorBufferInfo = {};
	descriptorBufferInfo.buffer = buffer;
	descriptorBufferInfo.offset = offset;
	descriptorBufferInfo.range = size;

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &descriptorBufferInfo;

	// TODO: not optimal, probably should be refactored to a Binder class,
	// i.e. it's better to collect all descriptor writes before the call
	vkUpdateDescriptorSets(device->getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanUtils::bindCombinedImageSampler(
	const render::backend::vulkan::Device *device,
	VkDescriptorSet descriptorSet,
	int binding,
	VkImageView imageView,
	VkSampler sampler
)
{
	VkDescriptorImageInfo descriptorImageInfo = {};
	descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	descriptorImageInfo.imageView = imageView;
	descriptorImageInfo.sampler = sampler;

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &descriptorImageInfo;

	// TODO: not optimal, probably should be refactored to a Binder class,
	// i.e. it's better to collect all descriptor writes before the call
	vkUpdateDescriptorSets(device->getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanUtils::copyBuffer(
	const render::backend::vulkan::Device *device,
	VkBuffer src,
	VkBuffer dst,
	VkDeviceSize size
)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

	endSingleTimeCommands(device, commandBuffer);
}


void VulkanUtils::copyBufferToImage(
	const render::backend::vulkan::Device *device,
	VkBuffer src,
	VkImage dst,
	uint32_t width,
	uint32_t height,
	uint32_t depth,
	uint32_t mipLevel,
	uint32_t layer,
	VkDeviceSize bufferOffset
)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

	VkBufferImageCopy region = {};
	region.bufferOffset = bufferOffset;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = mipLevel;
	region.imageSubresource.baseArrayLayer = layer;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = depth;

	vkCmdCopyBufferToImage(commandBuffer, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(device, commandBuffer);
}

/*
 */
bool VulkanUtils::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool VulkanUtils::isDepthFormat(VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_D16_UNORM: return true;
		case VK_FORMAT_D16_UNORM_S8_UINT: return true;
		case VK_FORMAT_D24_UNORM_S8_UINT: return true;
		case VK_FORMAT_D32_SFLOAT: return true;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return true;
		default: return false;
	}
}

void VulkanUtils::transitionImageLayout(
	const render::backend::vulkan::Device *device,
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t baseMipLevel,
	uint32_t numMipLevels,
	uint32_t baseLayer,
	uint32_t numLayers
)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = baseMipLevel;
	barrier.subresourceRange.levelCount = numMipLevels;
	barrier.subresourceRange.baseArrayLayer = baseLayer;
	barrier.subresourceRange.layerCount = numLayers;

	if (isDepthFormat(format))
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (hasStencilComponent(format))
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = 0;

		srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
	else
		throw std::runtime_error("Unsupported layout transition");

	vkCmdPipelineBarrier(
		commandBuffer,
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(device, commandBuffer);
}

void VulkanUtils::generateImage2DMipmaps(
	const render::backend::vulkan::Device *device,
	VkImage image,
	VkFormat imageFormat,
	uint32_t width,
	uint32_t height,
	uint32_t mipLevels,
	VkFormat format,
	VkFilter filter
)
{
	if (mipLevels == 1)
		return;

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device->getPhysicalDevice(), format, &formatProperties);

	bool supportsLinearFiltering = (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
	bool supportsCubicFiltering = (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_EXT) != 0;

	if (filter == VK_FILTER_LINEAR && !supportsLinearFiltering)
		throw std::runtime_error("Linear filtering is not supported on this device");

	if (filter == VK_FILTER_CUBIC_EXT && !supportsCubicFiltering)
		throw std::runtime_error("Cubic filtering is not supported on this device");

	// generate mips
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = width;
	int32_t mipHeight = height;

	for (uint32_t i = 1; i < mipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { std::max(1, mipWidth / 2), std::max(1, mipHeight / 2), 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(
			commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			filter
		);

		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		mipWidth = std::max(1, mipWidth / 2);
		mipHeight = std::max(1, mipHeight / 2);
	}

	endSingleTimeCommands(device, commandBuffer);
}

VkCommandBuffer VulkanUtils::beginSingleTimeCommands(const render::backend::vulkan::Device *device)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device->getCommandPool();
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device->getDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanUtils::endSingleTimeCommands(const render::backend::vulkan::Device *device, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;

	VkFence fence;
	if (vkCreateFence(device->getDevice(), &fenceInfo, nullptr, &fence) != VK_SUCCESS)
		throw std::runtime_error("Can't create fence");

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo, fence) != VK_SUCCESS)
		throw std::runtime_error("Can't submit command buffer");
	
	if (vkWaitForFences(device->getDevice(), 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
		throw std::runtime_error("Can't wait for a fence");

	vkDestroyFence(device->getDevice(), fence, nullptr);

	vkFreeCommandBuffers(device->getDevice(), device->getCommandPool(), 1, &commandBuffer);
}
