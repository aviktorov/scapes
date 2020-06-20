#pragma once

#include <volk.h>
#include <string>
#include <vector>

#include "VulkanCubemapRenderer.h"
#include "VulkanTexture2DRenderer.h"
#include "VulkanTexture.h"
#include "VulkanShader.h"

#include <render/backend/driver.h>

namespace render::backend::vulkan
{
	class Device;
}

class RenderScene;
class VulkanSwapChain;
struct VulkanRenderFrame;

/*
 */
class VulkanRenderer
{
public:
	VulkanRenderer(render::backend::Driver *driver,VkExtent2D extent);
	virtual ~VulkanRenderer();

	void init(const RenderScene *scene);
	void shutdown();
	void resize(const VulkanSwapChain *swapChain);
	void render(const RenderScene *scene, const VulkanRenderFrame &frame);

	void reload(const RenderScene *scene);
	void setEnvironment(const VulkanTexture *texture);

private:
	const render::backend::vulkan::Device *device {nullptr};
	render::backend::Driver *driver {nullptr};
	VkExtent2D extent;

	VulkanTexture2DRenderer bakedBRDFRenderer;
	VulkanCubemapRenderer hdriToCubeRenderer;
	std::vector<VulkanCubemapRenderer*> cubeToPrefilteredRenderers;
	VulkanCubemapRenderer diffuseIrradianceRenderer;

	VulkanTexture bakedBRDF;
	VulkanTexture environmentCubemap;
	VulkanTexture diffuseIrradianceCubemap;

	render::backend::BindSet *scene_bind_set {nullptr};
};
