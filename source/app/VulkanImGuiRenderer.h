#pragma once

#include <volk.h>

#include "VulkanRendererContext.h"

class RenderScene;
class VulkanSwapChain;
struct RendererState;
struct VulkanRenderFrame;

/*
 */
class VulkanImGuiRenderer
{
public:
	VulkanImGuiRenderer(
		const VulkanRendererContext &context,
		VkExtent2D extent,
		VkRenderPass renderPass
	);

	virtual ~VulkanImGuiRenderer();

	void init(const RenderScene *scene, const VulkanSwapChain *swapChain);
	void shutdown();
	void resize(const VulkanSwapChain *swapChain);
	void render(const RenderScene *scene, const VulkanRenderFrame &frame);

private:
	VulkanRendererContext context;
	VkExtent2D extent;
	VkRenderPass renderPass;
};
