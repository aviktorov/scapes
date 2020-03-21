#include "VulkanCubemapRenderer.h"
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
struct CubemapFaceOrientationData
{
	glm::mat4 faces[6];
};

/*
 */
void VulkanCubemapRenderer::init(
	const VulkanShader &vertexShader,
	const VulkanShader &fragmentShader,
	const VulkanTexture &targetTexture,
	int mip,
	uint32_t pushConstantsSize_
)
{
	pushConstantsSize = pushConstantsSize_;
	rendererQuad.createQuad(2.0f);

	for (int i = 0; i < 6; i++)
	{
		faceViews[i] = VulkanUtils::createImageView(
			context,
			targetTexture.getImage(),
			targetTexture.getImageFormat(),
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			mip, 1,
			i, 1
		);
	}

	targetExtent.width = targetTexture.getWidth(mip);
	targetExtent.height = targetTexture.getHeight(mip);

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

	VulkanDescriptorSetLayoutBuilder descriptorSetLayoutBuilder(context);
	descriptorSetLayout = descriptorSetLayoutBuilder
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.build();

	VulkanRenderPassBuilder renderPassBuilder(context);
	renderPass = renderPassBuilder
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorAttachment(targetTexture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.addColorAttachmentReference(0, 1)
		.addColorAttachmentReference(0, 2)
		.addColorAttachmentReference(0, 3)
		.addColorAttachmentReference(0, 4)
		.addColorAttachmentReference(0, 5)
		.build();

	VulkanPipelineLayoutBuilder pipelineLayoutBuilder(context);
	pipelineLayoutBuilder.addDescriptorSetLayout(descriptorSetLayout);

	if (pushConstantsSize > 0)
		pipelineLayoutBuilder.addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, pushConstantsSize);

	pipelineLayout = pipelineLayoutBuilder.build();

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
	descriptorSetAllocInfo.descriptorPool = context->getDescriptorPool();
	descriptorSetAllocInfo.descriptorSetCount = 1;
	descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;

	if (vkAllocateDescriptorSets(context->getDevice(), &descriptorSetAllocInfo, &descriptorSet) != VK_SUCCESS)
		throw std::runtime_error("Can't allocate descriptor sets");

	// Create framebuffer
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = 6;
	framebufferInfo.pAttachments = faceViews;
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

	// Fill uniform buffer
	CubemapFaceOrientationData *ubo = nullptr;
	vkMapMemory(context->getDevice(), uniformBufferMemory, 0, sizeof(CubemapFaceOrientationData), 0, reinterpret_cast<void**>(&ubo));

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

	vkUnmapMemory(context->getDevice(), uniformBufferMemory);

	// Bind data to descriptor set
	VulkanUtils::bindUniformBuffer(
		context,
		descriptorSet,
		0,
		uniformBuffer,
		0,
		uboSize
	);

	// Create fence
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;

	if (vkCreateFence(context->getDevice(), &fenceInfo, nullptr, &fence) != VK_SUCCESS)
		throw std::runtime_error("Can't create fence");
}

void VulkanCubemapRenderer::shutdown()
{
	vkDestroyBuffer(context->getDevice(), uniformBuffer, nullptr);
	uniformBuffer = VK_NULL_HANDLE;

	vkFreeMemory(context->getDevice(), uniformBufferMemory, nullptr);
	uniformBufferMemory = VK_NULL_HANDLE;

	vkDestroyFramebuffer(context->getDevice(), frameBuffer, nullptr);
	frameBuffer = VK_NULL_HANDLE;

	vkDestroyPipeline(context->getDevice(), pipeline, nullptr);
	pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(context->getDevice(), pipelineLayout, nullptr);
	pipelineLayout = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(context->getDevice(), descriptorSetLayout, nullptr);
	descriptorSetLayout = VK_NULL_HANDLE;

	vkDestroyRenderPass(context->getDevice(), renderPass, nullptr);
	renderPass = VK_NULL_HANDLE;

	for (int i = 0; i < 6; i++)
	{
		vkDestroyImageView(context->getDevice(), faceViews[i], nullptr);
		faceViews[i] = VK_NULL_HANDLE;
	}

	vkFreeCommandBuffers(context->getDevice(), context->getCommandPool(), 1, &commandBuffer);
	commandBuffer = VK_NULL_HANDLE;

	vkFreeDescriptorSets(context->getDevice(), context->getDescriptorPool(), 1, &descriptorSet);
	descriptorSet = VK_NULL_HANDLE;

	vkDestroyFence(context->getDevice(), fence, nullptr);
	fence = VK_NULL_HANDLE;

	rendererQuad.clearGPUData();
	rendererQuad.clearCPUData();
}

/*
 */
void VulkanCubemapRenderer::render(const VulkanTexture &inputTexture, float *pushConstants, int inputMip)
{
	VulkanUtils::bindCombinedImageSampler(
		context,
		descriptorSet,
		1,
		(inputMip == -1) ? inputTexture.getImageView() : inputTexture.getMipImageView(inputMip),
		inputTexture.getSampler()
	);

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

	if (pushConstantsSize > 0 && pushConstants)
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, pushConstantsSize, pushConstants);

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
