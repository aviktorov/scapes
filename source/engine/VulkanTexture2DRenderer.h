// TODO: remove Vulkan dependencies
#pragma once

#include <volk.h>
#include <string>
#include <vector>

#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanMesh.h"

#include <render/backend/driver.h>

class VulkanContext;

/*
 */
class VulkanTexture2DRenderer
{
public:
	VulkanTexture2DRenderer(render::backend::Driver *driver);

	void init(
		const VulkanShader &vertex_shader,
		const VulkanShader &fragment_shader,
		const VulkanTexture &target_texture
	);

	void shutdown();
	void render();

private:
	const VulkanContext *context {nullptr};
	render::backend::Driver *driver {nullptr};

	VulkanMesh quad;
	VkExtent2D target_extent;

	VkPipelineLayout pipeline_layout {VK_NULL_HANDLE};
	VkRenderPass render_pass {VK_NULL_HANDLE};
	VkPipeline pipeline {VK_NULL_HANDLE};

	VkCommandBuffer commandBuffer {VK_NULL_HANDLE};
	VkFence fence {VK_NULL_HANDLE};

	render::backend::FrameBuffer *framebuffer {nullptr};
};
