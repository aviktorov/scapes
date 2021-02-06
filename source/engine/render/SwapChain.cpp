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

		backend::Multisample max_samples = driver->getMaxSampleCount();
		swap_chain = driver->createSwapChain(native_window, width, height, max_samples);

		initFrames(width, height, ubo_size);
	}

	void SwapChain::resize(uint32_t w, uint32_t h)
	{
		shutdown();
		init(w, h, ubo_size);
	}

	void SwapChain::shutdown()
	{
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

			// Create commandbuffer
			frame.command_buffer = driver->createCommandBuffer(backend::CommandBufferType::PRIMARY);

			frame.swap_chain = swap_chain;
		}
	}

	void SwapChain::shutdownFrames()
	{
		for (RenderFrame &frame : frames)
		{
			driver->unmap(frame.uniform_buffer);
			driver->destroyUniformBuffer(frame.uniform_buffer);
			driver->destroyCommandBuffer(frame.command_buffer);
			driver->destroyBindSet(frame.bind_set);
		}
		frames.clear();
	}
}
