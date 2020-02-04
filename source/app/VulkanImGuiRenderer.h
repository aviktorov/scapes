#pragma once

#include <volk.h>

class RenderScene;
class VulkanContext;
class VulkanSwapChain;
class VulkanTexture;
struct VulkanRenderFrame;

/*
 */
class VulkanImGuiRenderer
{
public:
	VulkanImGuiRenderer(
		const VulkanContext *context,
		VkExtent2D extent,
		VkRenderPass renderPass
	);

	virtual ~VulkanImGuiRenderer();

	void *addTexture(const VulkanTexture &texture) const;

	void init(const RenderScene *scene, const VulkanSwapChain *swapChain);
	void shutdown();
	void resize(const VulkanSwapChain *swapChain);
	void render(const RenderScene *scene, const VulkanRenderFrame &frame);

private:
	const VulkanContext *context {nullptr};
	VkExtent2D extent;
	VkRenderPass renderPass;
};
