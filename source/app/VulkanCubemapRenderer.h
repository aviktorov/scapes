#pragma once

#include <volk.h>
#include <string>
#include <vector>

#include "VulkanRendererContext.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanMesh.h"

/*
 */
class VulkanCubemapRenderer
{
public:
	VulkanCubemapRenderer(const VulkanRendererContext &context)
		: context(context)
		, rendererVertexShader(context)
		, rendererFragmentShader(context)
		, rendererQuad(context)
	{ }

	void init(const VulkanTexture &inputTexture, const VulkanTexture &targetTexture);
	void shutdown();

	void render();

private:
	VulkanRendererContext context;
	VulkanShader rendererVertexShader;
	VulkanShader rendererFragmentShader;
	VulkanMesh rendererQuad;

	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptorSetLayout {VK_NULL_HANDLE};
	VkRenderPass renderPass {VK_NULL_HANDLE};
	VkPipeline pipeline {VK_NULL_HANDLE};

	VkFramebuffer frameBuffer {VK_NULL_HANDLE};
	VkCommandBuffer commandBuffer {VK_NULL_HANDLE};
	VkDescriptorSet descriptorSet {VK_NULL_HANDLE};

	// TODO: move to VkUniformBuffer<T>;
	VkBuffer uniformBuffer {VK_NULL_HANDLE};
	VkDeviceMemory uniformBufferMemory {VK_NULL_HANDLE};

	
};
