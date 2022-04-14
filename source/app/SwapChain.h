#pragma once

#include <vector>
#include <scapes/visual/hardware/Device.h>

/*
 */
class SwapChain
{
public:
	SwapChain(scapes::visual::hardware::Device *device, void *native_window);
	virtual ~SwapChain();

	void init();
	void recreate();
	void shutdown();

	scapes::visual::hardware::CommandBuffer acquire();
	bool present(scapes::visual::hardware::CommandBuffer command_buffer);

	inline scapes::visual::hardware::SwapChain getBackend() { return swap_chain; }

private:
	enum
	{
		NUM_IN_FLIGHT_FRAMES = 1,
	};

	scapes::visual::hardware::Device *device {nullptr};
	scapes::visual::hardware::SwapChain swap_chain {SCAPES_NULL_HANDLE};
	void *native_window {nullptr};

	uint32_t current_in_flight_frame {0};
	scapes::visual::hardware::CommandBuffer command_buffers[NUM_IN_FLIGHT_FRAMES];

	uint32_t width {0};
	uint32_t height {0};
};
