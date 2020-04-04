// TODO: remove Vulkan dependencies
#include "VulkanTexture2DRenderer.h"
#include "VulkanContext.h"

#include "VulkanDescriptorSetLayoutBuilder.h"
#include "VulkanGraphicsPipelineBuilder.h"
#include "VulkanPipelineLayoutBuilder.h"
#include "VulkanRenderPassBuilder.h"

#include "VulkanUtils.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

#include <render/backend/vulkan/driver.h>

/*
 */
void VulkanTexture2DRenderer::init(
	const VulkanShader &vertex_shader,
	const VulkanShader &fragment_shader,
	const VulkanTexture &target_texture
)
{
	quad.createQuad(2.0f);

	target_extent.width = target_texture.getWidth();
	target_extent.height = target_texture.getHeight();

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(target_extent.width);
	viewport.height = static_cast<float>(target_extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent.width = target_extent.width;
	scissor.extent.height = target_extent.height;

	VkShaderStageFlags stage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VulkanRenderPassBuilder render_pass_builder(context);
	render_pass = render_pass_builder
		.addColorAttachment(target_texture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.build();

	VulkanPipelineLayoutBuilder pipeline_layout_builder(context);
	pipeline_layout = pipeline_layout_builder
		.build();

	VulkanGraphicsPipelineBuilder pipeline_builder(context, pipeline_layout, render_pass);
	pipeline = pipeline_builder
		.addShaderStage(vertex_shader.getShaderModule(), VK_SHADER_STAGE_VERTEX_BIT)
		.addShaderStage(fragment_shader.getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT)
		.addVertexInput(VulkanMesh::getVertexInputBindingDescription(), VulkanMesh::getAttributeDescriptions())
		.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.addViewport(viewport)
		.addScissor(scissor)
		.setRasterizerState(false, false, VK_POLYGON_MODE_FILL, 1.0f, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.setMultisampleState(VK_SAMPLE_COUNT_1_BIT)
		.setDepthStencilState(false, false, VK_COMPARE_OP_LESS)
		.addBlendColorAttachment()
		.build();

	// Create framebuffer
	render::backend::FrameBufferColorAttachment attachments[] =
	{
		{ target_texture.getBackend(), 0, 1, 0, 1},
	};

	framebuffer = driver->createFrameBuffer(1, attachments);

	// Create command buffer
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool = context->getCommandPool();
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(context->getDevice(), &allocate_info, &commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Can't create command buffers");

	// Create fence
	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = 0;

	if (vkCreateFence(context->getDevice(), &fence_info, nullptr, &fence) != VK_SUCCESS)
		throw std::runtime_error("Can't create fence");
}

void VulkanTexture2DRenderer::shutdown()
{
	driver->destroyFrameBuffer(framebuffer);
	framebuffer = nullptr;

	vkDestroyPipeline(context->getDevice(), pipeline, nullptr);
	pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(context->getDevice(), pipeline_layout, nullptr);
	pipeline_layout = VK_NULL_HANDLE;

	vkDestroyRenderPass(context->getDevice(), render_pass, nullptr);
	render_pass = VK_NULL_HANDLE;

	vkFreeCommandBuffers(context->getDevice(), context->getCommandPool(), 1, &commandBuffer);
	commandBuffer = VK_NULL_HANDLE;

	vkDestroyFence(context->getDevice(), fence, nullptr);
	fence = VK_NULL_HANDLE;

	quad.clearGPUData();
	quad.clearCPUData();
}

/*
 */
void VulkanTexture2DRenderer::render()
{
	// Record command buffer
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("Can't begin recording command buffer");

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = render_pass;
	renderPassInfo.framebuffer = static_cast<render::backend::vulkan::FrameBuffer *>(framebuffer)->framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = target_extent.width;
	renderPassInfo.renderArea.extent.height = target_extent.height;

	VkClearValue clearValue = {};
	clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearValue;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	{
		VkBuffer vertexBuffers[] = { quad.getVertexBuffer() };
		VkBuffer indexBuffer = quad.getIndexBuffer();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer, quad.getNumIndices(), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Can't record command buffer");

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	if (vkResetFences(context->getDevice(), 1, &fence) != VK_SUCCESS)
		throw std::runtime_error("Can't reset fence");

	if (vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, fence) != VK_SUCCESS)
		throw std::runtime_error("Can't submit command buffer");

	if (vkWaitForFences(context->getDevice(), 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
		throw std::runtime_error("Can't wait for a fence");
}
