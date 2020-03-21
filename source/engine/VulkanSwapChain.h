#pragma once

#include <volk.h>
#include <vector>

class VulkanContext;

/*
 */
struct VulkanRenderFrame
{
	VkDescriptorSet descriptorSet {VK_NULL_HANDLE};

	VkFramebuffer frameBuffer {VK_NULL_HANDLE};
	VkCommandBuffer commandBuffer {VK_NULL_HANDLE};

	VkBuffer uniformBuffer {VK_NULL_HANDLE};
	VkDeviceMemory uniformBufferMemory {VK_NULL_HANDLE};
};

/*
 */
class VulkanSwapChain
{
public:
	VulkanSwapChain(const VulkanContext *context, void *nativeWindow, VkDeviceSize uboSize);
	virtual ~VulkanSwapChain();

	void init(int width, int height);
	void reinit(int width, int height);
	void shutdown();

	bool acquire(void *state, VulkanRenderFrame &frame);
	bool present(const VulkanRenderFrame &frame);

	inline uint32_t getNumImages() const { return static_cast<uint32_t>(swapChainImages.size()); }
	inline VkExtent2D getExtent() const { return swapChainExtent; }
	inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
	inline VkRenderPass getRenderPass() const { return renderPass; }
	inline VkRenderPass getNoClearRenderPass() const { return noClearRenderPass; }

private:
	struct SupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct Settings
	{
		VkSurfaceFormatKHR format;
		VkPresentModeKHR presentMode;
	};

	VulkanSwapChain::SupportDetails fetchSwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const;
	VulkanSwapChain::Settings selectOptimalSwapChainSettings(const VulkanSwapChain::SupportDetails &details) const;

private:
	void initTransient(int width, int height);
	void shutdownTransient();

	void initPersistent();
	void shutdownPersistent();

	void initFrames(VkDeviceSize uboSize);
	void shutdownFrames();

private:
	const VulkanContext *context {nullptr};
	std::vector<VulkanRenderFrame> frames;
	VkDeviceSize uboSize;

	//
	void *nativeWindow {nullptr};
	VkSurfaceKHR surface {VK_NULL_HANDLE};
	VkSwapchainKHR swapChain {VK_NULL_HANDLE};

	uint32_t presentQueueFamily {0xFFFF};
	VkQueue presentQueue {VK_NULL_HANDLE};

	VulkanSwapChain::SupportDetails details;
	VulkanSwapChain::Settings settings;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;

	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

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

	//
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	uint32_t imageIndex {0};
	uint32_t currentFrame {0};

	bool framebufferResized {false};
	enum
	{
		MAX_FRAMES_IN_FLIGHT = 2,
	};
};
