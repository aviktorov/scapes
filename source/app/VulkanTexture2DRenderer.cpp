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

/*
 */
void VulkanTexture2DRenderer::init(
	const VulkanShader &vertexShader,
	const VulkanShader &fragmentShader,
	const VulkanTexture &targetTexture
)
{
	rendererQuad.createQuad(2.0f);

	targetExtent.width = targetTexture.getWidth();
	targetExtent.height = targetTexture.getHeight();

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(targetExtent.width);
	viewport.height = static_cast<float>(targetExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent.width = targetExtent.width;
	scissor.extent.height = targetExtent.height;

	VkShaderStageFlags stage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VulkanRenderPassBuilder renderPassBuilder(context);
	renderPass = renderPassBuilder
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.build();

	VulkanPipelineLayoutBuilder pipelineLayoutBuilder(context);
	pipelineLayout = pipelineLayoutBuilder
		.build();

	VulkanGraphicsPipelineBuilder pipelineBuilder(context, pipelineLayout, renderPass);
	pipeline = pipelineBuilder
		.addShaderStage(vertexShader.getShaderModule(), VK_SHADER_STAGE_VERTEX_BIT)
		.addShaderStage(fragmentShader.getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT)
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
	std::array<VkImageView, 1> views = { targetTexture.getImageView() };

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
	framebufferInfo.pAttachments = views.data();
	framebufferInfo.width = targetExtent.width;
	framebufferInfo.height = targetExtent.height;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(context->getDevice(), &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS)
		throw std::runtime_error("Can't create framebuffer");

	// Create command buffer
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = context->getCommandPool();
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(context->getDevice(), &allocateInfo, &commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Can't create command buffers");

	// Create fence
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;

	if (vkCreateFence(context->getDevice(), &fenceInfo, nullptr, &fence) != VK_SUCCESS)
		throw std::runtime_error("Can't create fence");
}

void VulkanTexture2DRenderer::shutdown()
{
	vkDestroyFramebuffer(context->getDevice(), frameBuffer, nullptr);
	frameBuffer = VK_NULL_HANDLE;

	vkDestroyPipeline(context->getDevice(), pipeline, nullptr);
	pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(context->getDevice(), pipelineLayout, nullptr);
	pipelineLayout = VK_NULL_HANDLE;

	vkDestroyRenderPass(context->getDevice(), renderPass, nullptr);
	renderPass = VK_NULL_HANDLE;

	vkFreeCommandBuffers(context->getDevice(), context->getCommandPool(), 1, &commandBuffer);
	commandBuffer = VK_NULL_HANDLE;

	vkDestroyFence(context->getDevice(), fence, nullptr);
	fence = VK_NULL_HANDLE;

	rendererQuad.clearGPUData();
	rendererQuad.clearCPUData();
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
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = frameBuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = targetExtent.width;
	renderPassInfo.renderArea.extent.height = targetExtent.height;

	VkClearValue clearValue = {};
	clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearValue;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	{
		VkBuffer vertexBuffers[] = { rendererQuad.getVertexBuffer() };
		VkBuffer indexBuffer = rendererQuad.getIndexBuffer();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer, rendererQuad.getNumIndices(), 1, 0, 0, 0);
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
