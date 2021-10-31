#include "SwapChain.h"

/*
 */
SwapChain::SwapChain(scapes::foundation::render::Device *device, void *native_window)
	: device(device)
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
	swap_chain = device->createSwapChain(native_window);

	initFrames();
}

void SwapChain::recreate()
{
	device->destroySwapChain(swap_chain);
	swap_chain = device->createSwapChain(native_window);
}

void SwapChain::shutdown()
{
	shutdownFrames();

	device->destroySwapChain(swap_chain);
	swap_chain = nullptr;
}

/*
 */
scapes::foundation::render::CommandBuffer *SwapChain::acquire()
{
	uint32_t image_index = 0;
	if (!device->acquire(swap_chain, &image_index))
		return nullptr;

	scapes::foundation::render::CommandBuffer *command_buffer = command_buffers[current_in_flight_frame];
	device->wait(1, &command_buffer);

	return command_buffer;
}

bool SwapChain::present(scapes::foundation::render::CommandBuffer *command_buffer)
{
	bool result = device->present(swap_chain, 1, &command_buffer);
	current_in_flight_frame = (current_in_flight_frame + 1) % NUM_IN_FLIGHT_FRAMES;

	return result;
}

/*
 */
void SwapChain::initFrames()
{
	for (int i = 0; i < NUM_IN_FLIGHT_FRAMES; i++)
		command_buffers[i] = device->createCommandBuffer(scapes::foundation::render::CommandBufferType::PRIMARY);
}

void SwapChain::shutdownFrames()
{
	for (int i = 0; i < NUM_IN_FLIGHT_FRAMES; i++)
	{
		device->destroyCommandBuffer(command_buffers[i]);
		command_buffers[i] = nullptr;
	}
}
