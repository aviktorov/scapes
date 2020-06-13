#pragma once

#include <volk.h>
#include <render/backend/driver.h>

class RenderScene;
class VulkanSwapChain;
class VulkanTexture;
struct VulkanRenderFrame;
struct ImGuiContext;

namespace render::backend::vulkan
{
	class Device;
}


/*
 */
class VulkanImGuiRenderer
{
public:
	VulkanImGuiRenderer(
		render::backend::Driver *driver,
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
	render::backend::Driver *driver {nullptr};
	const render::backend::vulkan::Device *device {nullptr};
	ImGuiContext *imguiContext {nullptr};
	VkExtent2D extent;
	VkRenderPass renderPass;
};
