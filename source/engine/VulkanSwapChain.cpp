#include "VulkanSwapChain.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"

#include "VulkanDescriptorSetLayoutBuilder.h"
#include "VulkanRenderPassBuilder.h"

#include <array>
#include <algorithm>
#include <cassert>

#include <render/backend/vulkan/driver.h>

VulkanSwapChain::VulkanSwapChain(render::backend::Driver *driver, void *nativeWindow, VkDeviceSize uboSize)
	: driver(driver)
	, uboSize(uboSize)
	, native_window(nativeWindow)
{
	context = static_cast<render::backend::VulkanDriver *>(driver)->getContext();
}

VulkanSwapChain::~VulkanSwapChain()
{
	shutdown();
}

uint32_t VulkanSwapChain::getNumImages() const
{
	render::backend::vulkan::SwapChain *vk_swap_chain = reinterpret_cast<render::backend::vulkan::SwapChain *>(swap_chain);
	return vk_swap_chain->num_images;
}

VkExtent2D VulkanSwapChain::getExtent() const
{
	render::backend::vulkan::SwapChain *vk_swap_chain = reinterpret_cast<render::backend::vulkan::SwapChain *>(swap_chain);
	return vk_swap_chain->sizes;
}

void VulkanSwapChain::init(int width, int height)
{
	swap_chain = driver->createSwapChain(native_window, width, height);
	render::backend::vulkan::SwapChain *vk_swap_chain = reinterpret_cast<render::backend::vulkan::SwapChain *>(swap_chain);

	initPersistent(vk_swap_chain->surface_format.format);
	initTransient(width, height, vk_swap_chain->surface_format.format);
	initFrames(uboSize, width, height, vk_swap_chain->num_images, vk_swap_chain->views);
}

void VulkanSwapChain::reinit(int width, int height)
{
	shutdownTransient();
	shutdownFrames();

	driver->destroySwapChain(swap_chain);

	swap_chain = driver->createSwapChain(native_window, width, height);
	render::backend::vulkan::SwapChain *vk_swap_chain = reinterpret_cast<render::backend::vulkan::SwapChain *>(swap_chain);

	initTransient(width, height, vk_swap_chain->surface_format.format);
	initFrames(uboSize, width, height, vk_swap_chain->num_images, vk_swap_chain->views);
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
	render::backend::vulkan::SwapChain *vk_swap_chain = reinterpret_cast<render::backend::vulkan::SwapChain *>(swap_chain);
	uint32_t current_frame = vk_swap_chain->current_image;

	vkWaitForFences(
		context->getDevice(),
		1, &vk_swap_chain->rendering_finished_cpu[current_frame],
		VK_TRUE, std::numeric_limits<uint64_t>::max()
	);

	uint32_t image_index = 0;
	VkResult result = vkAcquireNextImageKHR(
		context->getDevice(),
		vk_swap_chain->swap_chain,
		std::numeric_limits<uint64_t>::max(),
		vk_swap_chain->image_available_gpu[current_frame],
		VK_NULL_HANDLE,
		&image_index
	);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		return false;

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("Can't acquire swap chain image");

	frame = frames[image_index];

	// copy render state to ubo
	memcpy(frame.uniformBufferData, state, static_cast<size_t>(uboSize));

	// reset command buffer
	if (vkResetCommandBuffer(frame.commandBuffer, 0) != VK_SUCCESS)
		throw std::runtime_error("Can't reset command buffer");

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	if (vkBeginCommandBuffer(frame.commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("Can't begin recording command buffer");

	return true;
}

bool VulkanSwapChain::present(const VulkanRenderFrame &frame)
{
	render::backend::vulkan::SwapChain *vk_swap_chain = reinterpret_cast<render::backend::vulkan::SwapChain *>(swap_chain);
	uint32_t current_frame = vk_swap_chain->current_image;

	if (vkEndCommandBuffer(frame.commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Can't record command buffer");
	
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &vk_swap_chain->image_available_gpu[current_frame];
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frame.commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &vk_swap_chain->rendering_finished_gpu[current_frame];

	vkResetFences(context->getDevice(), 1, &vk_swap_chain->rendering_finished_cpu[current_frame]);
	if (vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, vk_swap_chain->rendering_finished_cpu[current_frame]) != VK_SUCCESS)
		throw std::runtime_error("Can't submit command buffer");

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &vk_swap_chain->rendering_finished_gpu[current_frame];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &vk_swap_chain->swap_chain;
	presentInfo.pImageIndices = &current_frame;

	VulkanUtils::transitionImageLayout(
		context,
		vk_swap_chain->images[current_frame],
		vk_swap_chain->surface_format.format,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	);

	VkResult result = vkQueuePresentKHR(vk_swap_chain->present_queue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		return false;

	if (result != VK_SUCCESS)
		throw std::runtime_error("Can't present swap chain image");

	vk_swap_chain->current_image++;
	vk_swap_chain->current_image %= vk_swap_chain->num_images;

	return true;
}

/*
 */
void VulkanSwapChain::initTransient(int width, int height, VkFormat image_format)
{
	// Create color buffer & image view
	VulkanUtils::createImage2D(
		context,
		width,
		height,
		1,
		context->getMaxMSAASamples(),
		image_format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		colorImage,
		colorImageMemory
	);

	colorImageView = VulkanUtils::createImageView(
		context,
		colorImage,
		image_format,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D
	);

	VulkanUtils::transitionImageLayout(
		context,
		colorImage,
		image_format,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);

	// Create depth buffer & image view
	VulkanUtils::createImage2D(
		context,
		width,
		height,
		1,
		context->getMaxMSAASamples(),
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
	vkDestroyImageView(context->getDevice(), colorImageView, nullptr);
	colorImageView = VK_NULL_HANDLE;

	vkDestroyImage(context->getDevice(), colorImage, nullptr);
	colorImage = VK_NULL_HANDLE;

	vkFreeMemory(context->getDevice(), colorImageMemory, nullptr);
	colorImageMemory = VK_NULL_HANDLE;

	vkDestroyImageView(context->getDevice(), depthImageView, nullptr);
	depthImageView = VK_NULL_HANDLE;

	vkDestroyImage(context->getDevice(), depthImage, nullptr);
	depthImage = VK_NULL_HANDLE;

	vkFreeMemory(context->getDevice(), depthImageMemory, nullptr);
	depthImageMemory = VK_NULL_HANDLE;
}

/*
 */
void VulkanSwapChain::initPersistent(VkFormat image_format)
{
	assert(native_window);

	depthFormat = VulkanUtils::selectOptimalDepthFormat(context->getPhysicalDevice());

	// Create descriptor set layout and render pass
	VulkanDescriptorSetLayoutBuilder descriptorSetLayoutBuilder(context);
	descriptorSetLayout = descriptorSetLayoutBuilder
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL)
		.build();

	VulkanRenderPassBuilder renderPassBuilder(context);
	renderPass = renderPassBuilder
		.addColorAttachment(image_format, context->getMaxMSAASamples(), VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorResolveAttachment(image_format, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE)
		.addDepthStencilAttachment(depthFormat, context->getMaxMSAASamples(), VK_ATTACHMENT_LOAD_OP_CLEAR)
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.addColorResolveAttachmentReference(0, 1)
		.setDepthStencilAttachmentReference(0, 2)
		.build();

	VulkanRenderPassBuilder noClearRenderPassBuilder(context);
	noClearRenderPass = noClearRenderPassBuilder
		.addColorAttachment(image_format, context->getMaxMSAASamples(), VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorResolveAttachment(image_format, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE)
		.addDepthStencilAttachment(depthFormat, context->getMaxMSAASamples())
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.addColorResolveAttachmentReference(0, 1)
		.setDepthStencilAttachmentReference(0, 2)
		.build();
}

void VulkanSwapChain::shutdownPersistent()
{
	vkDestroyDescriptorSetLayout(context->getDevice(), descriptorSetLayout, nullptr);
	descriptorSetLayout = VK_NULL_HANDLE;

	vkDestroyRenderPass(context->getDevice(), renderPass, nullptr);
	renderPass = VK_NULL_HANDLE;

	vkDestroyRenderPass(context->getDevice(), noClearRenderPass, nullptr);
	noClearRenderPass = VK_NULL_HANDLE;
}

/*
 */
void VulkanSwapChain::initFrames(VkDeviceSize uboSize, uint32_t width, uint32_t height, uint32_t num_images, VkImageView *views)
{
	// Create uniform buffers
	frames.resize(num_images);

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

		vkMapMemory(context->getDevice(), frame.uniformBufferMemory, 0, uboSize, 0, &frame.uniformBufferData);

		// Create & fill descriptor set
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocInfo.descriptorPool = context->getDescriptorPool();
		descriptorSetAllocInfo.descriptorSetCount = 1;
		descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(context->getDevice(), &descriptorSetAllocInfo, &frame.descriptorSet) != VK_SUCCESS)
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
			views[i],
			depthImageView,
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(context->getDevice(), &framebufferInfo, nullptr, &frame.frameBuffer) != VK_SUCCESS)
			throw std::runtime_error("Can't create framebuffer");

		// Create commandbuffer
		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = context->getCommandPool();
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(context->getDevice(), &allocateInfo, &frame.commandBuffer) != VK_SUCCESS)
			throw std::runtime_error("Can't create command buffers");
	}
}

void VulkanSwapChain::shutdownFrames()
{
	for (VulkanRenderFrame &frame : frames)
	{
		vkFreeCommandBuffers(context->getDevice(), context->getCommandPool(), 1, &frame.commandBuffer);
		vkFreeDescriptorSets(context->getDevice(), context->getDescriptorPool(), 1, &frame.descriptorSet);
		vkUnmapMemory(context->getDevice(), frame.uniformBufferMemory);
		vkDestroyBuffer(context->getDevice(), frame.uniformBuffer, nullptr);
		vkFreeMemory(context->getDevice(), frame.uniformBufferMemory, nullptr);
		vkDestroyFramebuffer(context->getDevice(), frame.frameBuffer, nullptr);
	}
	frames.clear();
}
