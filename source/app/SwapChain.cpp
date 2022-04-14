#include "SwapChain.h"

/*
 */
SwapChain::SwapChain(scapes::visual::hardware::Device *device, void *native_window)
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

	for (int i = 0; i < NUM_IN_FLIGHT_FRAMES; i++)
		command_buffers[i] = device->createCommandBuffer(scapes::visual::hardware::CommandBufferType::PRIMARY);
}

void SwapChain::recreate()
{
	device->destroySwapChain(swap_chain);
	swap_chain = device->createSwapChain(native_window);
}

void SwapChain::shutdown()
{
	for (int i = 0; i < NUM_IN_FLIGHT_FRAMES; i++)
	{
		device->destroyCommandBuffer(command_buffers[i]);
		command_buffers[i] = SCAPES_NULL_HANDLE;
	}

	device->destroySwapChain(swap_chain);
	swap_chain = SCAPES_NULL_HANDLE;
}

/*
 */
scapes::visual::hardware::CommandBuffer SwapChain::acquire()
{
	scapes::visual::hardware::CommandBuffer command_buffer = command_buffers[current_in_flight_frame];
	if (!device->wait(1, &command_buffer))
		return SCAPES_NULL_HANDLE;

	uint32_t image_index = 0;
	if (!device->acquire(swap_chain, &image_index))
		return SCAPES_NULL_HANDLE;

	return command_buffer;
}

bool SwapChain::present(scapes::visual::hardware::CommandBuffer command_buffer)
{
	bool result = device->present(swap_chain, 1, &command_buffer);
	current_in_flight_frame = (current_in_flight_frame + 1) % NUM_IN_FLIGHT_FRAMES;

	return result;
}
