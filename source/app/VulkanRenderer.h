#pragma once

#include <volk.h>
#include <string>
#include <vector>

#include "VulkanCubemapRenderer.h"
#include "VulkanRendererContext.h"
#include "VulkanTexture.h"
#include "VulkanShader.h"

class RenderScene;
class VulkanSwapChain;

struct RendererState;
struct VulkanRenderFrame;

/*
 */
class VulkanRenderer
{
public:
	VulkanRenderer(
		const VulkanRendererContext &context,
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
	VulkanRendererContext context;
	VkExtent2D extent;
	VkRenderPass renderPass {VK_NULL_HANDLE};
	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};

	//
	VulkanCubemapRenderer hdriToCubeRenderer;
	VulkanCubemapRenderer diffuseIrradianceRenderer;

	VulkanTexture environmentCubemap;
	VulkanTexture diffuseIrradianceCubemap;

	VkPipeline skyboxPipeline {VK_NULL_HANDLE};
	VkPipeline pbrPipeline {VK_NULL_HANDLE};

	VkDescriptorSetLayout descriptorSetLayout {VK_NULL_HANDLE};
	VkDescriptorSetLayout sceneDescriptorSetLayout {VK_NULL_HANDLE};
	VkDescriptorSet sceneDescriptorSet {VK_NULL_HANDLE};
};
