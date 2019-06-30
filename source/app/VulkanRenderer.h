#pragma once

#include <volk.h>
#include <string>
#include <vector>

#include "VulkanRendererContext.h"
#include "RenderScene.h"

/*
 */
class Renderer
{
public:
	Renderer(const VulkanRendererContext &context)
		: context(context), data(context) { }

	void init(
		const std::string &vertexShaderFile,
		const std::string &fragmentShaderFile,
		const std::string &textureFile,
		const std::string &modelFile
	);

	VkCommandBuffer render(uint32_t imageIndex);
	void shutdown();

private:
	VkShaderModule createShader(const std::string &path) const;

private:
	RenderScene data;
	VulkanRendererContext context;

	VkRenderPass renderPass {VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptorSetLayout {VK_NULL_HANDLE};
	VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
	VkPipeline pipeline {VK_NULL_HANDLE};

	std::vector<VkFramebuffer> frameBuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	std::vector<VkDescriptorSet> descriptorSets;
};
