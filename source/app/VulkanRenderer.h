#pragma once

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <string>
#include <vector>

#include "VulkanRendererContext.h"
#include "VulkanRenderData.h"

/*
 */
class Renderer
{
public:
	Renderer(const RendererContext &context)
		: context(context), data(context) { }

	void init(
		const std::string &vertexShaderFile,
		const std::string &fragmentShaderFile,
		const std::string &textureFile
	);
	VkCommandBuffer render(uint32_t imageIndex);
	void shutdown();

private:
	VkShaderModule createShader(const std::string &path) const;

private:
	RenderData data;
	RendererContext context;

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
