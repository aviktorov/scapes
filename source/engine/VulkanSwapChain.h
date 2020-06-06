#pragma once

#include <volk.h>
#include <vector>

#include <render/backend/driver.h>

class VulkanContext;

/*
 */
struct VulkanRenderFrame
{
	VkDescriptorSet descriptorSet {VK_NULL_HANDLE};
	VkCommandBuffer commandBuffer {VK_NULL_HANDLE};

	// TODO: migrate to render::backend
	VkFramebuffer frameBuffer {VK_NULL_HANDLE};

	void *uniformBufferData {nullptr};
	VkBuffer uniformBuffer {VK_NULL_HANDLE};
	VkDeviceMemory uniformBufferMemory {VK_NULL_HANDLE};
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
	inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptor_set_layout; }
	inline VkRenderPass getRenderPass() const { return render_pass; }
	inline VkRenderPass getNoClearRenderPass() const { return noclear_render_pass; }

private:
	void initTransient(int width, int height, VkFormat image_format);
	void shutdownTransient();

	void initPersistent(VkFormat image_format);
	void shutdownPersistent();

	void initFrames(VkDeviceSize uboSize, uint32_t width, uint32_t height, uint32_t num_images, VkImageView *views);
	void shutdownFrames();

private:
	render::backend::Driver *driver {nullptr};
	render::backend::SwapChain *swap_chain {nullptr};
	void *native_window {nullptr};

	const VulkanContext *context {nullptr};

	std::vector<VulkanRenderFrame> frames;
	VkDeviceSize ubo_size;

	VkRenderPass render_pass {VK_NULL_HANDLE};
	VkRenderPass noclear_render_pass {VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout {VK_NULL_HANDLE};

	render::backend::Texture *color {nullptr};
	render::backend::Texture *depth {nullptr};
	render::backend::Format depth_format {render::backend::Format::UNDEFINED};
};
