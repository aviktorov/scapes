#include "VulkanCubemapRenderer.h"

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
struct CubemapFaceOrientationData
{
	glm::mat4 faces[6];
};

/*
 */
void VulkanCubemapRenderer::init(
	const VulkanShader &vertexShader,
	const VulkanShader &fragmentShader,
	const VulkanTexture &inputTexture,
	const VulkanTexture &targetTexture
)
{
	rendererQuad.createQuad(2.0f);

	for (int i = 0; i < 6; i++)
	{
		faceViews[i] = VulkanUtils::createImageView(
			context,
			targetTexture.getImage(),
			targetTexture.getImageFormat(),
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			0, targetTexture.getNumMipLevels(),
			i, 1
		);
	}

	VkImageView targetView = targetTexture.getImageView();

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(targetTexture.getWidth());
	viewport.height = static_cast<float>(targetTexture.getHeight());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent.width = targetTexture.getWidth();
	scissor.extent.height = targetTexture.getHeight();

	VkShaderStageFlags stage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VulkanDescriptorSetLayoutBuilder descriptorSetLayoutBuilder(context);
	descriptorSetLayout = descriptorSetLayoutBuilder
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.build();

	VulkanRenderPassBuilder renderPassBuilder(context);
	renderPass = renderPassBuilder
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT)
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT)
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT)
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT)
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT)
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT)
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.addColorAttachmentReference(0, 1)
		.addColorAttachmentReference(0, 2)
		.addColorAttachmentReference(0, 3)
		.addColorAttachmentReference(0, 4)
		.addColorAttachmentReference(0, 5)
		.build();

	VulkanPipelineLayoutBuilder pipelineLayoutBuilder(context);
	pipelineLayout = pipelineLayoutBuilder
		.addDescriptorSetLayout(descriptorSetLayout)
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
		.addBlendColorAttachment()
		.addBlendColorAttachment()
		.addBlendColorAttachment()
		.addBlendColorAttachment()
		.addBlendColorAttachment()
		.build();

	// Create uniform buffers
	VkDeviceSize uboSize = sizeof(CubemapFaceOrientationData);

	VulkanUtils::createBuffer(
		context,
		uboSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		uniformBuffer,
		uniformBufferMemory
	);

	// Create descriptor set
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.descriptorPool = context.descriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = 1;
	descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;

	if (vkAllocateDescriptorSets(context.device, &descriptorSetAllocInfo, &descriptorSet) != VK_SUCCESS)
		throw std::runtime_error("Can't allocate descriptor sets");

	// Create framebuffer
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = 6;
	framebufferInfo.pAttachments = faceViews;
	framebufferInfo.width = targetTexture.getWidth();
	framebufferInfo.height = targetTexture.getHeight();
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(context.device, &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS)
		throw std::runtime_error("Can't create framebuffer");

	// Create command buffer
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = context.commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(context.device, &allocateInfo, &commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Can't create command buffers");

	// Fill uniform buffer
	CubemapFaceOrientationData *ubo = nullptr;
	vkMapMemory(context.device, uniformBufferMemory, 0, sizeof(CubemapFaceOrientationData), 0, reinterpret_cast<void**>(&ubo));

	const glm::mat4 &translateZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	const glm::vec3 faceDirs[6] = {
		glm::vec3( 1.0f,  0.0f,  0.0f),
		glm::vec3(-1.0f,  0.0f,  0.0f),
		glm::vec3( 0.0f,  1.0f,  0.0f),
		glm::vec3( 0.0f, -1.0f,  0.0f),
		glm::vec3( 0.0f,  0.0f,  1.0f),
		glm::vec3( 0.0f,  0.0f, -1.0f),
	};

	const glm::vec3 faceUps[6] = {
		glm::vec3( 0.0f,  0.0f, -1.0f),
		glm::vec3( 0.0f,  0.0f,  1.0f),
		glm::vec3(-1.0f,  0.0f,  0.0f),
		glm::vec3(-1.0f,  0.0f,  0.0f),
		glm::vec3( 0.0f, -1.0f,  0.0f),
		glm::vec3( 0.0f, -1.0f,  0.0f),
	};

	const glm::mat4 faceRotations[6] = {
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::mat4(1.0f),
		glm::mat4(1.0f),
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	};

	for (int i = 0; i < 6; i++)
		ubo->faces[i] = faceRotations[i] * glm::lookAtRH(glm::vec3(0.0f), faceDirs[i], faceUps[i]) * translateZ;

	vkUnmapMemory(context.device, uniformBufferMemory);

	// Bind data to descriptor set
	VulkanUtils::bindUniformBuffer(
		context,
		descriptorSet,
		0,
		uniformBuffer,
		0,
		uboSize
	);

	VulkanUtils::bindCombinedImageSampler(
		context,
		descriptorSet,
		1,
		inputTexture.getImageView(),
		inputTexture.getSampler()
	);

	// Create fence
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;

	if (vkCreateFence(context.device, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
		throw std::runtime_error("Can't create fence");

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
	renderPassInfo.renderArea.extent.width = static_cast<uint32_t>(targetTexture.getWidth());
	renderPassInfo.renderArea.extent.height = static_cast<uint32_t>(targetTexture.getHeight());

	VkClearValue clearValues[6];
	for (int i = 0; i < 6; i++)
	{
		clearValues[i] = {};
		clearValues[i].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	}
	renderPassInfo.clearValueCount = 6;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
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
}

void VulkanCubemapRenderer::shutdown()
{
	vkDestroyBuffer(context.device, uniformBuffer, nullptr);
	uniformBuffer = VK_NULL_HANDLE;

	vkFreeMemory(context.device, uniformBufferMemory, nullptr);
	uniformBufferMemory = VK_NULL_HANDLE;

	vkDestroyFramebuffer(context.device, frameBuffer, nullptr);
	frameBuffer = VK_NULL_HANDLE;

	vkDestroyPipeline(context.device, pipeline, nullptr);
	pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(context.device, pipelineLayout, nullptr);
	pipelineLayout = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(context.device, descriptorSetLayout, nullptr);
	descriptorSetLayout = nullptr;

	vkDestroyRenderPass(context.device, renderPass, nullptr);
	renderPass = VK_NULL_HANDLE;

	for (int i = 0; i < 6; i++)
	{
		vkDestroyImageView(context.device, faceViews[i], nullptr);
		faceViews[i] = VK_NULL_HANDLE;
	}

	vkFreeCommandBuffers(context.device, context.commandPool, 1, &commandBuffer);
	commandBuffer = VK_NULL_HANDLE;

	vkFreeDescriptorSets(context.device, context.descriptorPool, 1, &descriptorSet);
	descriptorSet = VK_NULL_HANDLE;

	vkDestroyFence(context.device, fence, nullptr);
	fence = VK_NULL_HANDLE;

	rendererQuad.clearGPUData();
	rendererQuad.clearCPUData();
}

/*
 */
void VulkanCubemapRenderer::render()
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, fence) != VK_SUCCESS)
		throw std::runtime_error("Can't submit command buffer");

	if (vkWaitForFences(context.device, 1, &fence, VK_TRUE, 100000000000) != VK_SUCCESS)
		throw std::runtime_error("Can't wait for a fence");
}
