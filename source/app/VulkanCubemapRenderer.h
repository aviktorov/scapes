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
class VulkanCubemapRenderer
{
public:
	VulkanCubemapRenderer(const VulkanContext *context)
		: context(context)
		, rendererQuad(context)
	{ }

	void init(
		const VulkanShader &vertexShader,
		const VulkanShader &fragmentShader,
		const VulkanTexture &targetTexture,
		int mip,
		uint32_t userDataSize = 0
	);

	void shutdown();

	void render(const VulkanTexture &inputTexture, float *userData = nullptr, int inputMip = -1);

private:
	const VulkanContext *context {nullptr};
	VulkanMesh rendererQuad;
	VkExtent2D targetExtent;

	VkImageView faceViews[6] {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};

	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptorSetLayout {VK_NULL_HANDLE};
	VkRenderPass renderPass {VK_NULL_HANDLE};
	VkPipeline pipeline {VK_NULL_HANDLE};

	VkFramebuffer frameBuffer {VK_NULL_HANDLE};
	VkCommandBuffer commandBuffer {VK_NULL_HANDLE};
	VkDescriptorSet descriptorSet {VK_NULL_HANDLE};
	VkFence fence {VK_NULL_HANDLE};

	// TODO: move to VkUniformBuffer<T>;
	VkBuffer uniformBuffer {VK_NULL_HANDLE};
	VkDeviceMemory uniformBufferMemory {VK_NULL_HANDLE};

	uint32_t pushConstantsSize {0};
};
