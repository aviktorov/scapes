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

	VkFramebuffer frameBuffer {VK_NULL_HANDLE};
	VkCommandBuffer commandBuffer {VK_NULL_HANDLE};

	void *uniformBufferData {nullptr};
	VkBuffer uniformBuffer {VK_NULL_HANDLE};
	VkDeviceMemory uniformBufferMemory {VK_NULL_HANDLE};
};

/*
 */
class VulkanSwapChain
{
public:
	VulkanSwapChain(render::backend::Driver *driver, void *nativeWindow, VkDeviceSize uboSize);
	virtual ~VulkanSwapChain();

	void init(int width, int height);
	void reinit(int width, int height);
	void shutdown();

	bool acquire(void *state, VulkanRenderFrame &frame);
	bool present(const VulkanRenderFrame &frame);

	uint32_t getNumImages() const;
	VkExtent2D getExtent() const;
	inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
	inline VkRenderPass getRenderPass() const { return renderPass; }
	inline VkRenderPass getNoClearRenderPass() const { return noClearRenderPass; }

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
	VkDeviceSize uboSize;

	//
	VkImage colorImage {VK_NULL_HANDLE};
	VkImageView colorImageView {VK_NULL_HANDLE};
	VkDeviceMemory colorImageMemory {VK_NULL_HANDLE};

	VkImage depthImage {VK_NULL_HANDLE};
	VkImageView depthImageView {VK_NULL_HANDLE};
	VkDeviceMemory depthImageMemory {VK_NULL_HANDLE};

	VkFormat depthFormat;

	VkRenderPass renderPass {VK_NULL_HANDLE};
	VkRenderPass noClearRenderPass {VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptorSetLayout {VK_NULL_HANDLE};
};
