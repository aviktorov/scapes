#pragma once

#include <vector>
#include <render/backend/driver.h>

/*
 */
class SwapChain
{
public:
	SwapChain(render::backend::Driver *driver, void *native_window);
	virtual ~SwapChain();

	void init();
	void recreate();
	void shutdown();

	render::backend::CommandBuffer *acquire();
	bool present(render::backend::CommandBuffer *command_buffer);

	inline render::backend::SwapChain *getBackend() { return swap_chain; }

private:
	void initFrames();
	void shutdownFrames();

private:
	enum
	{
		NUM_IN_FLIGHT_FRAMES = 1,
	};

	render::backend::Driver *driver {nullptr};
	render::backend::SwapChain *swap_chain {nullptr};
	void *native_window {nullptr};

	uint32_t current_in_flight_frame {0};
	render::backend::CommandBuffer *command_buffers[NUM_IN_FLIGHT_FRAMES];

	uint32_t ubo_size {0};
	uint32_t width {0};
	uint32_t height {0};
};
