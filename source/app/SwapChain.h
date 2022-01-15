#pragma once

#include <vector>
#include <scapes/foundation/render/Device.h>

/*
 */
class SwapChain
{
public:
	SwapChain(scapes::foundation::render::Device *device, void *native_window);
	virtual ~SwapChain();

	void init();
	void recreate();
	void shutdown();

	scapes::foundation::render::CommandBuffer acquire();
	bool present(scapes::foundation::render::CommandBuffer command_buffer);

	inline scapes::foundation::render::SwapChain getBackend() { return swap_chain; }

private:
	enum
	{
		NUM_IN_FLIGHT_FRAMES = 1,
	};

	scapes::foundation::render::Device *device {nullptr};
	scapes::foundation::render::SwapChain swap_chain {SCAPES_NULL_HANDLE};
	void *native_window {nullptr};

	uint32_t current_in_flight_frame {0};
	scapes::foundation::render::CommandBuffer command_buffers[NUM_IN_FLIGHT_FRAMES];

	uint32_t width {0};
	uint32_t height {0};
};
