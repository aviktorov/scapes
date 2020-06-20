#include "VulkanSwapChain.h"

#include <array>
#include <algorithm>
#include <cassert>

#include "render/backend/vulkan/driver.h"

using namespace render::backend;

VulkanSwapChain::VulkanSwapChain(Driver *driver, void *native_window, VkDeviceSize ubo_size)
	: driver(driver)
	, ubo_size(ubo_size)
	, native_window(native_window)
{
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

VkRenderPass VulkanSwapChain::getDummyRenderPass() const
{
	vulkan::FrameBuffer *vk_frame_buffer = static_cast<vulkan::FrameBuffer *>(frames[0].frame_buffer);
	return vk_frame_buffer->dummy_render_pass;
}

void VulkanSwapChain::init(int width, int height)
{
	swap_chain = driver->createSwapChain(native_window, width, height);
	vulkan::SwapChain *vk_swap_chain = static_cast<vulkan::SwapChain *>(swap_chain);

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
}

/*
 */
bool VulkanSwapChain::acquire(void *state, VulkanRenderFrame &frame)
{
	uint32_t image_index = 0;
	if (!driver->acquire(swap_chain, &image_index))
		return false;

	frame = frames[image_index];
	beginFrame(state, frame);

	return true;
}

void VulkanSwapChain::beginFrame(void *state, const VulkanRenderFrame &frame)
{
	memcpy(frame.uniform_buffer_data, state, static_cast<size_t>(ubo_size));

	vulkan::SwapChain *vk_swap_chain = static_cast<vulkan::SwapChain *>(swap_chain);
	VkFramebuffer framebuffer = static_cast<vulkan::FrameBuffer *>(frame.frame_buffer)->framebuffer;
	VkCommandBuffer command_buffer = static_cast<vulkan::CommandBuffer *>(frame.command_buffer)->command_buffer;

	driver->resetCommandBuffer(frame.command_buffer);
	driver->beginCommandBuffer(frame.command_buffer);

	std::array<RenderPassClearValue, 3> clear_values = {};
	clear_values[0].color = {0.2f, 0.2f, 0.2f, 1.0f};
	clear_values[1].color = {0.0f, 0.0f, 0.0f, 1.0f};
	clear_values[2].depth_stencil = {1.0f, 0};

	std::array<RenderPassLoadOp, 3> load_ops = { RenderPassLoadOp::CLEAR, RenderPassLoadOp::DONT_CARE, RenderPassLoadOp::CLEAR };
	std::array<RenderPassStoreOp, 3> store_ops = { RenderPassStoreOp::STORE, RenderPassStoreOp::STORE, RenderPassStoreOp::DONT_CARE };

	RenderPassInfo info;
	info.load_ops = load_ops.data();
	info.store_ops = store_ops.data();
	info.clear_values = clear_values.data();

	driver->beginRenderPass(frame.command_buffer, frame.frame_buffer, &info);
}

void VulkanSwapChain::endFrame(const VulkanRenderFrame &frame)
{
	driver->endRenderPass(frame.command_buffer);
	driver->endCommandBuffer(frame.command_buffer);
	driver->submitSyncked(frame.command_buffer, swap_chain);
}

bool VulkanSwapChain::present(const VulkanRenderFrame &frame)
{
	endFrame(frame);
	return driver->present(swap_chain, 1, &frame.command_buffer);
}

/*
 */
void VulkanSwapChain::initTransient(int width, int height, VkFormat image_format)
{
	// TODO: remove vulkan specific stuff
	VulkanDriver *vk_driver = static_cast<VulkanDriver *>(driver);

	Multisample max_samples = driver->getMaxSampleCount();
	Format format = vk_driver->fromFormat(image_format);
	Format depth_format = driver->getOptimalDepthFormat();

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

		// Create bind set
		frame.bind_set = driver->createBindSet();
		driver->bindUniformBuffer(frame.bind_set, 0, frame.uniform_buffer);

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
		frame.command_buffer = driver->createCommandBuffer(CommandBufferType::PRIMARY);
	}
}

void VulkanSwapChain::shutdownFrames()
{
	for (VulkanRenderFrame &frame : frames)
	{
		driver->unmap(frame.uniform_buffer);
		driver->destroyUniformBuffer(frame.uniform_buffer);
		driver->destroyFrameBuffer(frame.frame_buffer);
		driver->destroyCommandBuffer(frame.command_buffer);
		driver->destroyBindSet(frame.bind_set);
	}
	frames.clear();
}
