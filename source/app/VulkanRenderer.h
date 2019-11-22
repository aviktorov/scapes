#pragma once

#include <volk.h>
#include <string>
#include <vector>

#include "VulkanCubemapRenderer.h"
#include "VulkanRendererContext.h"
#include "VulkanTexture.h"
#include "VulkanShader.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

/*
 */
struct RendererState
{
	glm::mat4 world;
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 cameraPosWS;
	float lerpUserValues {0.0f};
	float userMetalness {0.0f};
	float userRoughness {0.0f};
};

class RenderScene;

/*
 */
class Renderer
{
public:
	Renderer(const VulkanRendererContext &context, const VulkanSwapChainContext &swapChainContext)
		: context(context)
		, swapChainContext(swapChainContext)
		, hdriToCubeRenderer(context)
		, diffuseIrradianceRenderer(context)
		, environmentCubemap(context)
		, diffuseIrradianceCubemap(context)
	{ }

	void init(const RenderScene *scene);
	void update(const RenderScene *scene);
	VkCommandBuffer render(const RenderScene *scene, uint32_t imageIndex);
	void shutdown();

private:
	VulkanRendererContext context;
	VulkanSwapChainContext swapChainContext;

	VulkanCubemapRenderer hdriToCubeRenderer;
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

	RendererState state;
};
