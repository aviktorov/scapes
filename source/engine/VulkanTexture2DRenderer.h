#pragma once

#include <volk.h>
#include <string>
#include <vector>

#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanMesh.h"

class VulkanContext;

/*
 */
class VulkanTexture2DRenderer
{
public:
	VulkanTexture2DRenderer(const VulkanContext *context)
		: context(context)
		, rendererQuad(context)
	{ }

	void init(
		const VulkanShader &vertexShader,
		const VulkanShader &fragmentShader,
		const VulkanTexture &targetTexture
	);

	void shutdown();
	void render();

private:
	const VulkanContext *context {nullptr};
	VulkanMesh rendererQuad;
	VkExtent2D targetExtent;

	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
	VkRenderPass renderPass {VK_NULL_HANDLE};
	VkPipeline pipeline {VK_NULL_HANDLE};

	VkFramebuffer frameBuffer {VK_NULL_HANDLE};
	VkCommandBuffer commandBuffer {VK_NULL_HANDLE};
	VkFence fence {VK_NULL_HANDLE};
};
