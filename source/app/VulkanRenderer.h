#pragma once

#include <volk.h>
#include <string>
#include <vector>

#include "VulkanCubemapRenderer.h"
#include "VulkanRendererContext.h"
#include "VulkanTexture.h"
#include "VulkanShader.h"

class RenderScene;

/*
 */
class Renderer
{
public:
	Renderer(const VulkanRendererContext &context, const VulkanSwapChainContext &swapChainContext)
		: context(context)
		, swapChainContext(swapChainContext)
		, commonCubeVertexShader(context)
		, hdriToCubeFragmentShader(context)
		, hdriToCubeRenderer(context)
		, diffuseIrradianceFragmentShader(context)
		, diffuseIrradianceRenderer(context)
		, environmentCubemap(context)
		, diffuseIrradianceCubemap(context)
	{ }

	void init(const RenderScene *scene);

	VkCommandBuffer render(uint32_t imageIndex);
	void shutdown();

private:
	VulkanRendererContext context;
	VulkanSwapChainContext swapChainContext;

	VulkanShader commonCubeVertexShader;

	VulkanShader hdriToCubeFragmentShader;
	VulkanCubemapRenderer hdriToCubeRenderer;

	VulkanShader diffuseIrradianceFragmentShader;
	VulkanCubemapRenderer diffuseIrradianceRenderer;

	VulkanTexture environmentCubemap;
	VulkanTexture diffuseIrradianceCubemap;

	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptorSetLayout {VK_NULL_HANDLE};
	VkRenderPass renderPass {VK_NULL_HANDLE};

	VkPipeline skyboxPipeline {VK_NULL_HANDLE};
	VkPipeline pbrPipeline {VK_NULL_HANDLE};

	std::vector<VkFramebuffer> frameBuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	std::vector<VkDescriptorSet> descriptorSets;
};
