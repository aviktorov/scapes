#pragma once

#include <volk.h>

class RenderScene;
class VulkanContext;
class VulkanSwapChain;
class VulkanTexture;
struct VulkanRenderFrame;
struct ImGuiContext;

/*
 */
class VulkanImGuiRenderer
{
public:
	VulkanImGuiRenderer(
		const VulkanContext *context,
		ImGuiContext *imguiContext,
		VkExtent2D extent,
		VkRenderPass renderPass
	);

	virtual ~VulkanImGuiRenderer();

	void *addTexture(const VulkanTexture &texture) const;

	void init(const VulkanSwapChain *swapChain);
	void shutdown();
	void resize(const VulkanSwapChain *swapChain);
	void render(const VulkanRenderFrame &frame);

private:
	const VulkanContext *context {nullptr};
	ImGuiContext *imguiContext {nullptr};
	VkExtent2D extent;
	VkRenderPass renderPass;
};
