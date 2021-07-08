#include "SwapChain.h"

/*
 */
SwapChain::SwapChain(render::backend::Driver *driver, void *native_window)
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
void SwapChain::init()
{
	swap_chain = driver->createSwapChain(native_window);

	initFrames();
}

void SwapChain::recreate()
{
	driver->destroySwapChain(swap_chain);
	swap_chain = driver->createSwapChain(native_window);
}

void SwapChain::shutdown()
{
	shutdownFrames();

	driver->destroySwapChain(swap_chain);
	swap_chain = nullptr;
}

/*
 */
render::backend::CommandBuffer *SwapChain::acquire()
{
	uint32_t image_index = 0;
	if (!driver->acquire(swap_chain, &image_index))
		return nullptr;

	render::backend::CommandBuffer *command_buffer = command_buffers[current_in_flight_frame];
	driver->wait(1, &command_buffer);

	return command_buffer;
}

bool SwapChain::present(render::backend::CommandBuffer *command_buffer)
{
	bool result = driver->present(swap_chain, 1, &command_buffer);
	current_in_flight_frame = (current_in_flight_frame + 1) % NUM_IN_FLIGHT_FRAMES;

	return result;
}

/*
 */
void SwapChain::initFrames()
{
	for (int i = 0; i < NUM_IN_FLIGHT_FRAMES; i++)
		command_buffers[i] = driver->createCommandBuffer(render::backend::CommandBufferType::PRIMARY);
}

void SwapChain::shutdownFrames()
{
	for (int i = 0; i < NUM_IN_FLIGHT_FRAMES; i++)
	{
		driver->destroyCommandBuffer(command_buffers[i]);
		command_buffers[i] = nullptr;
	}
}
