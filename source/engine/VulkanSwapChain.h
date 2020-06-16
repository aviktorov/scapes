#pragma once

#include <volk.h>
#include <vector>

#include <render/backend/driver.h>

namespace render::backend::vulkan
{
	class Device;
}

/*
 */
struct VulkanRenderFrame
{
	VkDescriptorSet descriptor_set {VK_NULL_HANDLE};

	render::backend::CommandBuffer *command_buffer {nullptr};
	render::backend::FrameBuffer *frame_buffer {nullptr};
	render::backend::UniformBuffer *uniform_buffer {nullptr};
	void *uniform_buffer_data {nullptr};
};

/*
 */
class VulkanSwapChain
{
public:
	VulkanSwapChain(render::backend::Driver *driver, void *native_window, VkDeviceSize ubo_size);
	virtual ~VulkanSwapChain();

	void init(int width, int height);
	void reinit(int width, int height);
	void shutdown();

	bool acquire(void *state, VulkanRenderFrame &frame);
	bool present(const VulkanRenderFrame &frame);

	uint32_t getNumImages() const;
	VkExtent2D getExtent() const;

	// TODO: remove this, it breaks main rendering
	VkRenderPass getDummyRenderPass() const;

	inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptor_set_layout; }

private:
	void beginFrame(void *state, const VulkanRenderFrame &frame);
	void endFrame(const VulkanRenderFrame &frame);

	void initTransient(int width, int height, VkFormat image_format);
	void shutdownTransient();

	void initPersistent(VkFormat image_format);
	void shutdownPersistent();

	void initFrames(VkDeviceSize uboSize, uint32_t width, uint32_t height, uint32_t num_images);
	void shutdownFrames();

private:
	render::backend::Driver *driver {nullptr};
	render::backend::SwapChain *swap_chain {nullptr};
	void *native_window {nullptr};

	const render::backend::vulkan::Device *device {nullptr};

	std::vector<VulkanRenderFrame> frames;
	VkDeviceSize ubo_size;

	VkDescriptorSetLayout descriptor_set_layout {VK_NULL_HANDLE};

	render::backend::Texture *color {nullptr};
	render::backend::Texture *depth {nullptr};
};
