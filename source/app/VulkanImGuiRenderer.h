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

	void init(const RendererState *state, const RenderScene *scene, const VulkanSwapChain *swapChain);
	void resize(const VulkanSwapChain *swapChain);
	void update(RendererState *state, const RenderScene *scene);
	void render(const RendererState *state, const RenderScene *scene, const VulkanRenderFrame &frame);
	void shutdown();

private:
	VulkanRendererContext context;
	VkExtent2D extent;
	VkRenderPass renderPass;
};
