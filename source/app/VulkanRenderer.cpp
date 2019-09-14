#include "VulkanMesh.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "VulkanGraphicsPipelineBuilder.h"

#include "RenderScene.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

#include <chrono>

/*
 */
struct SharedRendererState
{
	glm::mat4 world;
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 cameraPos;
};

/*
 */
void Renderer::init(const RenderScene *scene)
{
	// Create graphics pipeline
	const VulkanShader &vertexShader = scene->getVertexShader();
	const VulkanShader &fragmentShader = scene->getFragmentShader();

	VulkanGraphicsPipelineBuilder builder(context);

	builder.addShaderStage(vertexShader.getShaderModule(), VK_SHADER_STAGE_VERTEX_BIT);
	builder.addShaderStage(fragmentShader.getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT);
	builder.addVertexInput(VulkanMesh::getVertexInputBindingDescription(), VulkanMesh::getAttributeDescriptions());
	builder.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	
	// Create viewport state
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainContext.extent.width);
	viewport.height = static_cast<float>(swapChainContext.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = swapChainContext.extent;

	builder.addViewport(viewport);
	builder.addScissor(scissor);

	builder.setRasterizerState(false, false, VK_POLYGON_MODE_FILL, 1.0f, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	builder.setMultisampleState(context.maxMSAASamples, true);
	builder.setDepthStencilState(true, true, VK_COMPARE_OP_LESS);
	builder.addBlendColorAttachment();

	VkShaderStageFlags stage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	builder.addDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage);

	for (uint32_t i = 1; i < 7; i++)
		builder.addDescriptorSetLayoutBinding(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage);

	builder.addColorAttachment(swapChainContext.colorFormat, context.maxMSAASamples);
	builder.addDepthStencilAttachment(swapChainContext.depthFormat, context.maxMSAASamples);

	builder.build();

	renderPass = builder.getRenderPass();
	descriptorSetLayout = builder.getDescriptorSetLayout();
	pipelineLayout = builder.getPipelineLayout();
	pipeline = builder.getPipeline();

	// Create uniform buffers
	VkDeviceSize uboSize = sizeof(SharedRendererState);

	uint32_t imageCount = static_cast<uint32_t>(swapChainContext.swapChainImageViews.size());
	uniformBuffers.resize(imageCount);
	uniformBuffersMemory.resize(imageCount);

	for (uint32_t i = 0; i < imageCount; i++)
	{
		VulkanUtils::createBuffer(
			context,
			uboSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffers[i],
			uniformBuffersMemory[i]
		);
	}

	// Create descriptor sets
	std::vector<VkDescriptorSetLayout> layouts(imageCount, descriptorSetLayout);

	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.descriptorPool = swapChainContext.descriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = imageCount;
	descriptorSetAllocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(imageCount);
	if (vkAllocateDescriptorSets(context.device, &descriptorSetAllocInfo, descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Can't allocate descriptor sets");

	for (size_t i = 0; i < imageCount; i++)
	{
		std::array<const VulkanTexture *, 6> textures =
		{
			&scene->getAlbedoTexture(),
			&scene->getNormalTexture(),
			&scene->getAOTexture(),
			&scene->getShadingTexture(),
			&scene->getEmissionTexture(),
			&scene->getHDRTexture()
		};

		VulkanUtils::bindUniformBuffer(
			context,
			descriptorSets[i],
			0,
			uniformBuffers[i],
			0,
			sizeof(SharedRendererState)
		);

		for (int k = 0; k < textures.size(); k++)
			VulkanUtils::bindCombinedImageSampler(
				context,
				descriptorSets[i],
				k + 1,
				textures[k]->getImageView(),
				textures[k]->getSampler()
			);
	}

	// Create framebuffers
	frameBuffers.resize(imageCount);
	for (size_t i = 0; i < imageCount; i++) {
		std::array<VkImageView, 3> attachments = {
			swapChainContext.colorImageView,
			swapChainContext.swapChainImageViews[i],
			swapChainContext.depthImageView,
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainContext.extent.width;
		framebufferInfo.height = swapChainContext.extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(context.device, &framebufferInfo, nullptr, &frameBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Can't create framebuffer");
	}

	// Create command buffers
	commandBuffers.resize(imageCount);

	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = context.commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(context.device, &allocateInfo, commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("Can't create command buffers");

	// Record command buffers
	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Can't begin recording command buffer");

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = frameBuffers[i];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapChainContext.extent;

		std::array<VkClearValue, 3> clearValues = {};
		clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
		clearValues[1].color = {0.0f, 0.0f, 0.0f, 1.0f};
		clearValues[2].depthStencil = {1.0f, 0};
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

		const VulkanMesh &mesh = scene->getMesh();

		VkBuffer vertexBuffers[] = { mesh.getVertexBuffer() };
		VkBuffer indexBuffer = mesh.getIndexBuffer();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffers[i], mesh.getNumIndices(), 1, 0, 0, 0);
		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Can't record command buffer");
	}
}

void Renderer::shutdown()
{
	for (auto uniformBuffer : uniformBuffers)
		vkDestroyBuffer(context.device, uniformBuffer, nullptr);
	
	uniformBuffers.clear();

	for (auto uniformBufferMemory : uniformBuffersMemory)
		vkFreeMemory(context.device, uniformBufferMemory, nullptr);

	uniformBuffersMemory.clear();

	for (auto framebuffer : frameBuffers)
		vkDestroyFramebuffer(context.device, framebuffer, nullptr);

	frameBuffers.clear();

	vkDestroyPipeline(context.device, pipeline, nullptr);
	pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(context.device, pipelineLayout, nullptr);
	pipelineLayout = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(context.device, descriptorSetLayout, nullptr);
	descriptorSetLayout = nullptr;

	vkDestroyRenderPass(context.device, renderPass, nullptr);
	renderPass = VK_NULL_HANDLE;
}

/*
 */
VkCommandBuffer Renderer::render(uint32_t imageIndex)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();

	const float rotationSpeed = 0.1f;
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	VkBuffer uniformBuffer = uniformBuffers[imageIndex];
	VkDeviceMemory uniformBufferMemory = uniformBuffersMemory[imageIndex];

	const glm::vec3 &up = {0.0f, 0.0f, 1.0f};
	const glm::vec3 &zero = {0.0f, 0.0f, 0.0f};

	const float aspect = swapChainContext.extent.width / (float) swapChainContext.extent.height;
	const float zNear = 0.1f;
	const float zFar = 10.0f;

	SharedRendererState ubo = {};
	ubo.world = glm::rotate(glm::mat4(1.0f), time * rotationSpeed * glm::radians(90.0f), up);
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), zero, up);
	ubo.proj = glm::perspective(glm::radians(45.0f), aspect, zNear, zFar);
	ubo.proj[1][1] *= -1;
	ubo.cameraPos = glm::vec3(2.0f, 2.0f, 2.0f);

	void *data;
	vkMapMemory(context.device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(context.device, uniformBufferMemory);

	return commandBuffers[imageIndex];
}
