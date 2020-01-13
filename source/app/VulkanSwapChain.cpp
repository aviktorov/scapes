#include "VulkanSwapChain.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"

#include "VulkanDescriptorSetLayoutBuilder.h"
#include "VulkanRenderPassBuilder.h"

#include <array>
#include <algorithm>
#include <cassert>

VulkanSwapChain::VulkanSwapChain(const VulkanContext *context, VkDeviceSize uboSize)
	: context(context)
	, uboSize(uboSize)
{
}

VulkanSwapChain::~VulkanSwapChain()
{
	shutdown();
}

void VulkanSwapChain::init(int width, int height)
{
	initTransient(width, height);
	initPersistent();
	initFrames(uboSize);
}

void VulkanSwapChain::reinit(int width, int height)
{
	shutdownTransient();
	shutdownFrames();

	initTransient(width, height);
	initFrames(uboSize);
}

void VulkanSwapChain::shutdown()
{
	shutdownTransient();
	shutdownFrames();
	shutdownPersistent();
}

/*
 */
bool VulkanSwapChain::acquire(void *state, VulkanRenderFrame &frame)
{
	vkWaitForFences(context->device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	VkResult result = vkAcquireNextImageKHR(
		context->device,
		swapChain,
		std::numeric_limits<uint64_t>::max(),
		imageAvailableSemaphores[currentFrame],
		VK_NULL_HANDLE,
		&imageIndex
	);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		return false;

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("Can't acquire swap chain image");

	frame = frames[imageIndex];

	// copy render state to ubo
	void *ubo = nullptr;
	vkMapMemory(context->device, frame.uniformBufferMemory, 0, uboSize, 0, &ubo);
	memcpy(ubo, state, uboSize);
	vkUnmapMemory(context->device, frame.uniformBufferMemory);

	// reset command buffer
	if (vkResetCommandBuffer(frame.commandBuffer, 0) != VK_SUCCESS)
		throw std::runtime_error("Can't reset command buffer");

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(frame.commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("Can't begin recording command buffer");

	return true;
}

bool VulkanSwapChain::present(const VulkanRenderFrame &frame)
{
	if (vkEndCommandBuffer(frame.commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Can't record command buffer");
	
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frame.commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(context->device, 1, &inFlightFences[currentFrame]);
	if (vkQueueSubmit(context->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("Can't submit command buffer");

	VkSwapchainKHR swapChains[] = { swapChain };
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	VulkanUtils::transitionImageLayout(
		context,
		swapChainImages[imageIndex],
		swapChainImageFormat,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	);

	VkResult result = vkQueuePresentKHR(context->presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		return false;

	if (result != VK_SUCCESS)
		throw std::runtime_error("Can't present swap chain image");

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	return true;
}

/*
 */
VulkanSwapChain::SupportDetails VulkanSwapChain::fetchSwapChainSupportDetails() const
{
	VulkanSwapChain::SupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, context->surface, &details.capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, context->surface, &formatCount, nullptr);

	if (formatCount > 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, context->surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(context->physicalDevice, context->surface, &presentModeCount, nullptr);

	if (presentModeCount > 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(context->physicalDevice, context->surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

/*
 */
VulkanSwapChain::Settings VulkanSwapChain::selectOptimalSwapChainSettings(const VulkanSwapChain::SupportDetails &details, int width, int height) const
{
	assert(!details.formats.empty());
	assert(!details.presentModes.empty());

	VulkanSwapChain::Settings settings;

	// Select the best format if the surface has no preferred format
	if (details.formats.size() == 1 && details.formats[0].format == VK_FORMAT_UNDEFINED)
	{
		settings.format = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	// Otherwise, select one of the available formats
	else
	{
		settings.format = details.formats[0];
		for (const auto &format : details.formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				settings.format = format;
				break;
			}
		}
	}

	// Select the best present mode
	settings.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto &presentMode : details.presentModes)
	{
		// Some drivers currently don't properly support FIFO present mode,
		// so we should prefer IMMEDIATE mode if MAILBOX mode is not available
		if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			settings.presentMode = presentMode;

		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			settings.presentMode = presentMode;
			break;
		}
	}

	// Select current swap extent if window manager doesn't allow to set custom extent
	if (details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		settings.extent = details.capabilities.currentExtent;
	}
	// Otherwise, manually set extent to match the min/max extent bounds
	else
	{
		const VkSurfaceCapabilitiesKHR &capabilities = details.capabilities;

		settings.extent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		settings.extent.width = std::max(
			capabilities.minImageExtent.width,
			std::min(settings.extent.width, capabilities.maxImageExtent.width)
		);
		settings.extent.height = std::max(
			capabilities.minImageExtent.height,
			std::min(settings.extent.height, capabilities.maxImageExtent.height)
		);
	}

	return settings;
}

/*
 */
void VulkanSwapChain::initTransient(int width, int height)
{
	// Create swap chain
	VulkanSwapChain::SupportDetails details = fetchSwapChainSupportDetails();
	VulkanSwapChain::Settings settings = selectOptimalSwapChainSettings(details, width, height);

	// Simply sticking to this minimum means that we may sometimes have to wait
	// on the driver to complete internal operations before we can acquire another image to render to.
	// Therefore it is recommended to request at least one more image than the minimum
	uint32_t imageCount = details.capabilities.minImageCount + 1;

	// We should also make sure to not exceed the maximum number of images while doing this,
	// where 0 is a special value that means that there is no maximum
	if(details.capabilities.maxImageCount > 0)
		imageCount = std::min(imageCount, details.capabilities.maxImageCount);

	VkSwapchainCreateInfoKHR swapChainInfo = {};
	swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.surface = context->surface;
	swapChainInfo.minImageCount = imageCount;
	swapChainInfo.imageFormat = settings.format.format;
	swapChainInfo.imageColorSpace = settings.format.colorSpace;
	swapChainInfo.imageExtent = settings.extent;
	swapChainInfo.imageArrayLayers = 1;
	swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (context->graphicsQueueFamily != context->presentQueueFamily)
	{
		uint32_t queueFamilies[] = { context->graphicsQueueFamily, context->presentQueueFamily };
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainInfo.queueFamilyIndexCount = 2;
		swapChainInfo.pQueueFamilyIndices = queueFamilies;
	}
	else
	{
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainInfo.queueFamilyIndexCount = 0;
		swapChainInfo.pQueueFamilyIndices = nullptr;
	}

	swapChainInfo.preTransform = details.capabilities.currentTransform;
	swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainInfo.presentMode = settings.presentMode;
	swapChainInfo.clipped = VK_TRUE;
	swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(context->device, &swapChainInfo, nullptr, &swapChain) != VK_SUCCESS)
		throw std::runtime_error("Can't create swapchain");

	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(context->device, swapChain, &swapChainImageCount, nullptr);
	assert(swapChainImageCount != 0);

	swapChainImages.resize(swapChainImageCount);
	vkGetSwapchainImagesKHR(context->device, swapChain, &swapChainImageCount, swapChainImages.data());

	swapChainImageFormat = settings.format.format;
	swapChainExtent = settings.extent;

	// Create swap chain image views
	swapChainImageViews.resize(swapChainImageCount);
	for (size_t i = 0; i < swapChainImageViews.size(); i++)
		swapChainImageViews[i] = VulkanUtils::createImageView(
			context,
			swapChainImages[i],
			swapChainImageFormat,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_VIEW_TYPE_2D
		);

	// Create color buffer & image view
	VulkanUtils::createImage2D(
		context,
		swapChainExtent.width,
		swapChainExtent.height,
		1,
		context->maxMSAASamples,
		swapChainImageFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		colorImage,
		colorImageMemory
	);

	colorImageView = VulkanUtils::createImageView(
		context,
		colorImage,
		swapChainImageFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D
	);

	VulkanUtils::transitionImageLayout(
		context,
		colorImage,
		swapChainImageFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);

	// Create depth buffer & image view
	depthFormat = VulkanUtils::selectOptimalDepthFormat(context->physicalDevice);

	VulkanUtils::createImage2D(
		context,
		swapChainExtent.width,
		swapChainExtent.height,
		1,
		context->maxMSAASamples,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depthImage,
		depthImageMemory
	);

	depthImageView = VulkanUtils::createImageView(
		context,
		depthImage,
		depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_VIEW_TYPE_2D
	);

	VulkanUtils::transitionImageLayout(
		context,
		depthImage,
		depthFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	);
}

void VulkanSwapChain::shutdownTransient()
{
	vkDestroyImageView(context->device, colorImageView, nullptr);
	colorImageView = VK_NULL_HANDLE;

	vkDestroyImage(context->device, colorImage, nullptr);
	colorImage = VK_NULL_HANDLE;

	vkFreeMemory(context->device, colorImageMemory, nullptr);
	colorImageMemory = VK_NULL_HANDLE;

	vkDestroyImageView(context->device, depthImageView, nullptr);
	depthImageView = VK_NULL_HANDLE;

	vkDestroyImage(context->device, depthImage, nullptr);
	depthImage = VK_NULL_HANDLE;

	vkFreeMemory(context->device, depthImageMemory, nullptr);
	depthImageMemory = VK_NULL_HANDLE;

	for (auto imageView : swapChainImageViews)
		vkDestroyImageView(context->device, imageView, nullptr);

	swapChainImageViews.clear();
	swapChainImages.clear();

	vkDestroySwapchainKHR(context->device, swapChain, nullptr);
	swapChain = VK_NULL_HANDLE;
}

/*
 */
void VulkanSwapChain::initPersistent()
{
	// Create sync objects
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(context->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS)
			throw std::runtime_error("Can't create 'image available' semaphore");
		
		if (vkCreateSemaphore(context->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS)
			throw std::runtime_error("Can't create 'render finished' semaphore");

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateFence(context->device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("Can't create in flight frame fence");
	}

	// Create descriptor set layout and render pass
	VulkanDescriptorSetLayoutBuilder descriptorSetLayoutBuilder(context);
	descriptorSetLayout = descriptorSetLayoutBuilder
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL)
		.build();

	VulkanRenderPassBuilder renderPassBuilder(context);
	renderPass = renderPassBuilder
		.addColorAttachment(swapChainImageFormat, context->maxMSAASamples, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorResolveAttachment(swapChainImageFormat, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE)
		.addDepthStencilAttachment(depthFormat, context->maxMSAASamples, VK_ATTACHMENT_LOAD_OP_CLEAR)
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.addColorResolveAttachmentReference(0, 1)
		.setDepthStencilAttachmentReference(0, 2)
		.build();

	VulkanRenderPassBuilder noClearRenderPassBuilder(context);
	noClearRenderPass = noClearRenderPassBuilder
		.addColorAttachment(swapChainImageFormat, context->maxMSAASamples, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorResolveAttachment(swapChainImageFormat, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE)
		.addDepthStencilAttachment(depthFormat, context->maxMSAASamples)
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.addColorResolveAttachmentReference(0, 1)
		.setDepthStencilAttachmentReference(0, 2)
		.build();

}

void VulkanSwapChain::shutdownPersistent()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(context->device, imageAvailableSemaphores[i], nullptr);
		imageAvailableSemaphores[i] = VK_NULL_HANDLE;

		vkDestroySemaphore(context->device, renderFinishedSemaphores[i], nullptr);
		renderFinishedSemaphores[i] = VK_NULL_HANDLE;

		vkDestroyFence(context->device, inFlightFences[i], nullptr);
		inFlightFences[i] = VK_NULL_HANDLE;
	}

	vkDestroyDescriptorSetLayout(context->device, descriptorSetLayout, nullptr);
	descriptorSetLayout = nullptr;

	vkDestroyRenderPass(context->device, renderPass, nullptr);
	renderPass = VK_NULL_HANDLE;

	vkDestroyRenderPass(context->device, noClearRenderPass, nullptr);
	noClearRenderPass = VK_NULL_HANDLE;
}

/*
 */
void VulkanSwapChain::initFrames(VkDeviceSize uboSize)
{
	// Create uniform buffers
	uint32_t imageCount = static_cast<uint32_t>(swapChainImages.size());
	frames.resize(imageCount);

	for (int i = 0; i < frames.size(); i++)
	{
		VulkanRenderFrame &frame = frames[i];

		// Create uniform buffer object
		VulkanUtils::createBuffer(
			context,
			uboSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			frame.uniformBuffer,
			frame.uniformBufferMemory
		);

		// Create & fill descriptor set
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocInfo.descriptorPool = context->descriptorPool;
		descriptorSetAllocInfo.descriptorSetCount = 1;
		descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(context->device, &descriptorSetAllocInfo, &frame.descriptorSet) != VK_SUCCESS)
			throw std::runtime_error("Can't allocate swap chain descriptor sets");

		VulkanUtils::bindUniformBuffer(
			context,
			frame.descriptorSet,
			0,
			frame.uniformBuffer,
			0,
			uboSize
		);

		// Create framebuffer
		std::array<VkImageView, 3> attachments = {
			colorImageView,
			swapChainImageViews[i],
			depthImageView,
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(context->device, &framebufferInfo, nullptr, &frame.frameBuffer) != VK_SUCCESS)
			throw std::runtime_error("Can't create framebuffer");
		// Create commandbuffer
		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = context->commandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(context->device, &allocateInfo, &frame.commandBuffer) != VK_SUCCESS)
			throw std::runtime_error("Can't create command buffers");
	}
}

void VulkanSwapChain::shutdownFrames()
{
	for (VulkanRenderFrame &frame : frames)
	{
		vkFreeCommandBuffers(context->device, context->commandPool, 1, &frame.commandBuffer);
		vkFreeDescriptorSets(context->device, context->descriptorPool, 1, &frame.descriptorSet);
		vkDestroyBuffer(context->device, frame.uniformBuffer, nullptr);
		vkFreeMemory(context->device, frame.uniformBufferMemory, nullptr);
		vkDestroyFramebuffer(context->device, frame.frameBuffer, nullptr);
	}
	frames.clear();
}
