#pragma once

#include <vector>
#include <render/backend/driver.h>

/*
 */
struct RenderFrame
{
	render::backend::SwapChain *swap_chain {nullptr};
	render::backend::BindSet *bindings {nullptr};
	render::backend::CommandBuffer *command_buffer {nullptr};
	render::backend::UniformBuffer *uniform_buffer {nullptr};
	void *uniform_buffer_data {nullptr};
};

/*
 */
class SwapChain
{
public:
	SwapChain(render::backend::Driver *driver, void *native_window);
	virtual ~SwapChain();

	void init(uint32_t width, uint32_t height, uint32_t ubo_size = 0);
	void resize(uint32_t width, uint32_t height);
	void shutdown();

	bool acquire(RenderFrame &frame);
	bool present(const RenderFrame &frame);

	inline uint32_t getWidth() const { return width; }
	inline uint32_t getHeight() const { return height; }

	inline const render::backend::SwapChain *getBackend() const { return swap_chain; }

private:
	void initFrames(uint32_t width, uint32_t height, uint32_t ubo_size);
	void shutdownFrames();

private:
	render::backend::Driver *driver {nullptr};
	render::backend::SwapChain *swap_chain {nullptr};
	void *native_window {nullptr};

	std::vector<RenderFrame> frames;

	uint32_t ubo_size {0};
	uint32_t width {0};
	uint32_t height {0};
};
