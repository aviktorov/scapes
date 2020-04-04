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
class VulkanCubemapRenderer
{
public:
	VulkanCubemapRenderer(const VulkanContext *context, render::backend::Driver *driver)
		: context(context)
		, driver(driver)
		, quad(driver)
	{ }

	void init(
		const VulkanShader &vertex_shader,
		const VulkanShader &fragment_shader,
		const VulkanTexture &target_texture,
		int target_mip,
		uint32_t push_constants_size = 0
	);

	void shutdown();

	void render(const VulkanTexture &input_texture, float *push_constants = nullptr, int input_mip = -1);

private:
	const VulkanContext *context {nullptr};
	render::backend::Driver *driver {nullptr};

	VulkanMesh quad;
	VkExtent2D target_extent;

	VkPipelineLayout pipeline_layout {VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout {VK_NULL_HANDLE};
	VkRenderPass render_pass {VK_NULL_HANDLE};
	VkPipeline pipeline {VK_NULL_HANDLE};

	VkCommandBuffer command_buffer {VK_NULL_HANDLE};
	VkDescriptorSet descriptor_set {VK_NULL_HANDLE};
	VkFence fence {VK_NULL_HANDLE};

	render::backend::FrameBuffer *framebuffer {nullptr};
	render::backend::UniformBuffer *uniform_buffer {nullptr};

	uint32_t push_constants_size {0};
};
