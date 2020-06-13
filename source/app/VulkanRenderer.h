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
	VulkanRenderer(
		render::backend::Driver *driver,
		VkExtent2D extent,
		VkDescriptorSetLayout descriptorSetLayout,
		VkRenderPass renderPass
	);

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
	VkRenderPass renderPass {VK_NULL_HANDLE};
	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};

	VulkanTexture2DRenderer bakedBRDFRenderer;
	VulkanCubemapRenderer hdriToCubeRenderer;
	std::vector<VulkanCubemapRenderer*> cubeToPrefilteredRenderers;
	VulkanCubemapRenderer diffuseIrradianceRenderer;

	VulkanTexture bakedBRDF;
	VulkanTexture environmentCubemap;
	VulkanTexture diffuseIrradianceCubemap;

	VkPipeline skyboxPipeline {VK_NULL_HANDLE};
	VkPipeline pbrPipeline {VK_NULL_HANDLE};

	VkDescriptorSetLayout descriptorSetLayout {VK_NULL_HANDLE};
	VkDescriptorSetLayout sceneDescriptorSetLayout {VK_NULL_HANDLE};
	VkDescriptorSet sceneDescriptorSet {VK_NULL_HANDLE};
};
