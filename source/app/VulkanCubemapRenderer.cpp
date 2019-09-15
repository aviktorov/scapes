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

static std::string rendererVertexShaderPath = "D:/Development/Projects/pbr-sandbox/shaders/cubemapRenderer.vert";
static std::string rendererFragmentShaderPath = "D:/Development/Projects/pbr-sandbox/shaders/cubemapRenderer.frag";

/*
 */
struct CubemapFaceOrientationData
{
	glm::mat3 faces[6];
};

/*
 */
void VulkanCubemapRenderer::init(const VulkanTexture &inputTexture, const VulkanTexture &targetTexture)
{
	rendererVertexShader.compileFromFile(rendererVertexShaderPath, VulkanShaderKind::Vertex);
	rendererFragmentShader.compileFromFile(rendererFragmentShaderPath, VulkanShaderKind::Fragment);
	rendererQuad.createQuad(2.0f);

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
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.build();

	VulkanPipelineLayoutBuilder pipelineLayoutBuilder(context);
	pipelineLayout = pipelineLayoutBuilder
		.addDescriptorSetLayout(descriptorSetLayout)
		.build();

	VulkanGraphicsPipelineBuilder pipelineBuilder(context, pipelineLayout, renderPass);
	pipeline = pipelineBuilder
		.addShaderStage(rendererVertexShader.getShaderModule(), VK_SHADER_STAGE_VERTEX_BIT)
		.addShaderStage(rendererFragmentShader.getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT)
		.addVertexInput(VulkanMesh::getVertexInputBindingDescription(), VulkanMesh::getAttributeDescriptions())
		.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.addViewport(viewport)
		.addScissor(scissor)
		.setRasterizerState(false, false, VK_POLYGON_MODE_FILL, 1.0f, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.setMultisampleState(VK_SAMPLE_COUNT_1_BIT)
		.setDepthStencilState(false, false, VK_COMPARE_OP_LESS)
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
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &targetView;
	framebufferInfo.width = targetTexture.getWidth();
	framebufferInfo.height = targetTexture.getHeight();
	framebufferInfo.layers = targetTexture.getNumLayers();

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

	for (int i = 0; i < 6; i++)
	{
		ubo->faces[i] = glm::mat3(1.0f);
	}

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

	VkClearValue clearValue = {};
	clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearValue;

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

	rendererFragmentShader.clear();
}

/*
 */
void VulkanCubemapRenderer::render()
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(context.graphicsQueue);
}
