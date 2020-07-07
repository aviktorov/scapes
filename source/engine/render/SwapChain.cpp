#include "SwapChain.h"

namespace render
{
	/*
	 */
	SwapChain::SwapChain(backend::Driver *driver, void *native_window)
		: driver(driver)
		, native_window(native_window)
	{
	}

	SwapChain::~SwapChain()
	{
		shutdown();
	}

	/*
	 */
	void SwapChain::init(uint32_t w, uint32_t h, uint32_t s)
	{
		ubo_size = s;
		width = w;
		height = h;

		swap_chain = driver->createSwapChain(native_window, width, height);

		initTransient(width, height);
		initFrames(width, height, ubo_size);
	}

	void SwapChain::resize(uint32_t w, uint32_t h)
	{
		shutdown();
		init(w, h, ubo_size);
	}

	void SwapChain::shutdown()
	{
		shutdownTransient();
		shutdownFrames();

		driver->destroySwapChain(swap_chain);
		swap_chain = nullptr;
	}

	/*
	 */
	bool SwapChain::acquire(RenderFrame &frame)
	{
		uint32_t image_index = 0;
		if (!driver->acquire(swap_chain, &image_index))
			return false;

		frame = frames[image_index];
		return true;
	}

	bool SwapChain::present(const RenderFrame &frame)
	{
		return driver->present(swap_chain, 1, &frame.command_buffer);
	}

	/*
	 */
	void SwapChain::initTransient(int width, int height)
	{
		backend::Multisample max_samples = driver->getMaxSampleCount();
		backend::Format depth_format = driver->getOptimalDepthFormat();
		backend::Format image_format = driver->getSwapChainImageFormat(swap_chain);

		color = driver->createTexture2D(width, height, 1, image_format, max_samples);
		depth = driver->createTexture2D(width, height, 1, depth_format, max_samples);
	}

	void SwapChain::shutdownTransient()
	{
		driver->destroyTexture(color);
		color = nullptr;

		driver->destroyTexture(depth);
		depth = nullptr;
	}

	/*
	 */
	void SwapChain::initFrames(uint32_t width, uint32_t height, uint32_t ubo_size)
	{
		uint32_t num_images = driver->getNumSwapChainImages(swap_chain);

		// Create uniform buffers
		frames.resize(num_images);

		for (int i = 0; i < frames.size(); i++)
		{
			RenderFrame &frame = frames[i];
			frame = {};

			// Create uniform buffer object
			if (ubo_size > 0)
			{
				frame.uniform_buffer = driver->createUniformBuffer(backend::BufferType::DYNAMIC, ubo_size);
				frame.uniform_buffer_data = driver->map(frame.uniform_buffer);
			}

			// Create bind set
			frame.bind_set = driver->createBindSet();

			if (ubo_size > 0)
				driver->bindUniformBuffer(frame.bind_set, 0, frame.uniform_buffer);

			// Create framebuffer
			backend::FrameBufferAttachment attachments[3] = { {}, {}, {} };
			attachments[0].type = backend::FrameBufferAttachmentType::COLOR;
			attachments[0].color.texture = color;
			attachments[1].type = backend::FrameBufferAttachmentType::SWAP_CHAIN_COLOR;
			attachments[1].swap_chain_color.swap_chain = swap_chain;
			attachments[1].swap_chain_color.base_image = i;
			attachments[1].swap_chain_color.resolve_attachment = true;
			attachments[2].type = backend::FrameBufferAttachmentType::DEPTH;
			attachments[2].depth.texture = depth;

			frame.frame_buffer = driver->createFrameBuffer(3, attachments);

			// Create commandbuffer
			frame.command_buffer = driver->createCommandBuffer(backend::CommandBufferType::PRIMARY);
		}
	}

	void SwapChain::shutdownFrames()
	{
		for (RenderFrame &frame : frames)
		{
			driver->unmap(frame.uniform_buffer);
			driver->destroyUniformBuffer(frame.uniform_buffer);
			driver->destroyFrameBuffer(frame.frame_buffer);
			driver->destroyCommandBuffer(frame.command_buffer);
			driver->destroyBindSet(frame.bind_set);
		}
		frames.clear();
	}
}
