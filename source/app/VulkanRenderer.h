#pragma once

#include <volk.h>
#include <string>
#include <vector>

#include "VulkanRendererContext.h"

class RenderScene;

/*
 */
class Renderer
{
public:
	Renderer(const VulkanRendererContext &context, const VulkanSwapChainContext &swapChainContext)
		: context(context), swapChainContext(swapChainContext) { }

	void init(const RenderScene *scene);

	VkCommandBuffer render(uint32_t imageIndex);
	void shutdown();

private:
	VulkanRendererContext context;
	VulkanSwapChainContext swapChainContext;

	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
	VkPipeline pipeline {VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptorSetLayout {VK_NULL_HANDLE};
	VkRenderPass renderPass {VK_NULL_HANDLE};

	std::vector<VkFramebuffer> frameBuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	std::vector<VkDescriptorSet> descriptorSets;
};
