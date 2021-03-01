#pragma once

#include <vector>
#include <render/backend/driver.h>

namespace render
{
	/*
	 */
	struct RenderFrame
	{
		backend::SwapChain *swap_chain {nullptr};
		backend::BindSet *bindings {nullptr};
		backend::CommandBuffer *command_buffer {nullptr};
		backend::UniformBuffer *uniform_buffer {nullptr};
		void *uniform_buffer_data {nullptr};
	};

	/*
	 */
	class SwapChain
	{
	public:
		SwapChain(backend::Driver *driver, void *native_window);
		virtual ~SwapChain();

		void init(uint32_t width, uint32_t height, uint32_t ubo_size = 0);
		void resize(uint32_t width, uint32_t height);
		void shutdown();

		bool acquire(RenderFrame &frame);
		bool present(const RenderFrame &frame);

		inline uint32_t getWidth() const { return width; }
		inline uint32_t getHeight() const { return height; }

		inline const backend::SwapChain *getBackend() const { return swap_chain; }

	private:
		void initFrames(uint32_t width, uint32_t height, uint32_t ubo_size);
		void shutdownFrames();

	private:
		backend::Driver *driver {nullptr};
		backend::SwapChain *swap_chain {nullptr};
		void *native_window {nullptr};

		std::vector<RenderFrame> frames;

		uint32_t ubo_size {0};
		uint32_t width {0};
		uint32_t height {0};
	};
}
