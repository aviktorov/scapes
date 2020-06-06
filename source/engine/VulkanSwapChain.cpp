#include "VulkanSwapChain.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"

#include "VulkanDescriptorSetLayoutBuilder.h"
#include "VulkanRenderPassBuilder.h"

#include <array>
#include <algorithm>
#include <cassert>

#include <render/backend/vulkan/driver.h>

using namespace render::backend;

VulkanSwapChain::VulkanSwapChain(Driver *driver, void *native_window, VkDeviceSize ubo_size)
	: driver(driver)
	, ubo_size(ubo_size)
	, native_window(native_window)
{
	context = static_cast<VulkanDriver *>(driver)->getContext();
}

VulkanSwapChain::~VulkanSwapChain()
{
	shutdown();
}

uint32_t VulkanSwapChain::getNumImages() const
{
	vulkan::SwapChain *vk_swap_chain = static_cast<vulkan::SwapChain *>(swap_chain);
	return vk_swap_chain->num_images;
}

VkExtent2D VulkanSwapChain::getExtent() const
{
	vulkan::SwapChain *vk_swap_chain = static_cast<vulkan::SwapChain *>(swap_chain);
	return vk_swap_chain->sizes;
}

void VulkanSwapChain::init(int width, int height)
{
	swap_chain = driver->createSwapChain(native_window, width, height);
	vulkan::SwapChain *vk_swap_chain = static_cast<vulkan::SwapChain *>(swap_chain);

	initPersistent(vk_swap_chain->surface_format.format);
	initTransient(width, height, vk_swap_chain->surface_format.format);
	initFrames(ubo_size, width, height, vk_swap_chain->num_images);
}

void VulkanSwapChain::reinit(int width, int height)
{
	shutdownTransient();
	shutdownFrames();

	driver->destroySwapChain(swap_chain);

	swap_chain = driver->createSwapChain(native_window, width, height);
	vulkan::SwapChain *vk_swap_chain = static_cast<vulkan::SwapChain *>(swap_chain);

	initTransient(width, height, vk_swap_chain->surface_format.format);
	initFrames(ubo_size, width, height, vk_swap_chain->num_images);
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
	vulkan::SwapChain *vk_swap_chain = static_cast<vulkan::SwapChain *>(swap_chain);
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
	memcpy(frame.uniform_buffer_data, state, static_cast<size_t>(ubo_size));

	// reset command buffer
	if (vkResetCommandBuffer(frame.command_buffer, 0) != VK_SUCCESS)
		throw std::runtime_error("Can't reset command buffer");

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	if (vkBeginCommandBuffer(frame.command_buffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("Can't begin recording command buffer");

	return true;
}

bool VulkanSwapChain::present(const VulkanRenderFrame &frame)
{
	vulkan::SwapChain *vk_swap_chain = static_cast<vulkan::SwapChain *>(swap_chain);
	uint32_t current_frame = vk_swap_chain->current_image;

	if (vkEndCommandBuffer(frame.command_buffer) != VK_SUCCESS)
		throw std::runtime_error("Can't record command buffer");
	
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &vk_swap_chain->image_available_gpu[current_frame];
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frame.command_buffer;
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
	// TODO: remove vulkan specific stuff
	VulkanDriver *vk_driver = static_cast<VulkanDriver *>(driver);

	Multisample max_samples = driver->getMaxSampleCount();
	Format format = vk_driver->fromFormat(image_format);

	color = driver->createTexture2D(width, height, 1, format, max_samples);
	depth = driver->createTexture2D(width, height, 1, depth_format, max_samples);
}

void VulkanSwapChain::shutdownTransient()
{
	driver->destroyTexture(color);
	color = nullptr;

	driver->destroyTexture(depth);
	depth = nullptr;
}

/*
 */
void VulkanSwapChain::initPersistent(VkFormat image_format)
{
	assert(native_window);

	depth_format = driver->getOptimalDepthFormat();
	Multisample samples = driver->getMaxSampleCount();

	// TODO: remove vulkan specific stuff
	VulkanDriver *vk_driver = static_cast<VulkanDriver *>(driver);
	VkFormat vk_depth_format = vk_driver->toFormat(depth_format);
	VkSampleCountFlagBits vk_samples = vk_driver->toMultisample(samples);

	// Create descriptor set layout and render pass
	VulkanDescriptorSetLayoutBuilder descriptor_set_layout_builder(context);
	descriptor_set_layout = descriptor_set_layout_builder
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL)
		.build();

	VulkanRenderPassBuilder render_pass_builder(context);
	render_pass = render_pass_builder
		.addColorAttachment(image_format, vk_samples, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorResolveAttachment(image_format, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE)
		.addDepthStencilAttachment(vk_depth_format, vk_samples, VK_ATTACHMENT_LOAD_OP_CLEAR)
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.addColorResolveAttachmentReference(0, 1)
		.setDepthStencilAttachmentReference(0, 2)
		.build();

	VulkanRenderPassBuilder noclear_render_pass_builder(context);
	noclear_render_pass = noclear_render_pass_builder
		.addColorAttachment(image_format, vk_samples, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorResolveAttachment(image_format, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE)
		.addDepthStencilAttachment(vk_depth_format, vk_samples)
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.addColorResolveAttachmentReference(0, 1)
		.setDepthStencilAttachmentReference(0, 2)
		.build();
}

void VulkanSwapChain::shutdownPersistent()
{
	vkDestroyDescriptorSetLayout(context->getDevice(), descriptor_set_layout, nullptr);
	descriptor_set_layout = VK_NULL_HANDLE;

	vkDestroyRenderPass(context->getDevice(), render_pass, nullptr);
	render_pass = VK_NULL_HANDLE;

	vkDestroyRenderPass(context->getDevice(), noclear_render_pass, nullptr);
	noclear_render_pass = VK_NULL_HANDLE;
}

/*
 */
void VulkanSwapChain::initFrames(VkDeviceSize ubo_size, uint32_t width, uint32_t height, uint32_t num_images)
{
	// Create uniform buffers
	frames.resize(num_images);

	for (int i = 0; i < frames.size(); i++)
	{
		VulkanRenderFrame &frame = frames[i];

		// Create uniform buffer object
		frame.uniform_buffer = driver->createUniformBuffer(BufferType::DYNAMIC, static_cast<uint32_t>(ubo_size));
		frame.uniform_buffer_data = driver->map(frame.uniform_buffer);

		// Create & fill descriptor set
		VkDescriptorSetAllocateInfo ds_alloc_info = {};
		ds_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		ds_alloc_info.descriptorPool = context->getDescriptorPool();
		ds_alloc_info.descriptorSetCount = 1;
		ds_alloc_info.pSetLayouts = &descriptor_set_layout;

		if (vkAllocateDescriptorSets(context->getDevice(), &ds_alloc_info, &frame.descriptor_set) != VK_SUCCESS)
			throw std::runtime_error("Can't allocate swap chain descriptor sets");

		VkBuffer ubo = static_cast<vulkan::UniformBuffer *>(frame.uniform_buffer)->buffer;
		VulkanUtils::bindUniformBuffer(
			context,
			frame.descriptor_set,
			0,
			ubo,
			0,
			ubo_size
		);

		// Create framebuffer
		FrameBufferAttachment attachments[3] = { {}, {}, {} };
		attachments[0].type = FrameBufferAttachmentType::COLOR;
		attachments[0].color.texture = color;
		attachments[1].type = FrameBufferAttachmentType::SWAP_CHAIN_COLOR;
		attachments[1].swap_chain_color.swap_chain = swap_chain;
		attachments[1].swap_chain_color.base_image = i;
		attachments[1].swap_chain_color.resolve_attachment = true;
		attachments[2].type = FrameBufferAttachmentType::DEPTH;
		attachments[2].depth.texture = depth;
		
		frame.frame_buffer = driver->createFrameBuffer(3, attachments);

		// Create commandbuffer
		VkCommandBufferAllocateInfo cb_alloc_info = {};
		cb_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cb_alloc_info.commandPool = context->getCommandPool();
		cb_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cb_alloc_info.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(context->getDevice(), &cb_alloc_info, &frame.command_buffer) != VK_SUCCESS)
			throw std::runtime_error("Can't create command buffers");
	}
}

void VulkanSwapChain::shutdownFrames()
{
	for (VulkanRenderFrame &frame : frames)
	{
		vkFreeCommandBuffers(context->getDevice(), context->getCommandPool(), 1, &frame.command_buffer);
		vkFreeDescriptorSets(context->getDevice(), context->getDescriptorPool(), 1, &frame.descriptor_set);

		driver->unmap(frame.uniform_buffer);
		driver->destroyUniformBuffer(frame.uniform_buffer);
		driver->destroyFrameBuffer(frame.frame_buffer);
	}
	frames.clear();
}
