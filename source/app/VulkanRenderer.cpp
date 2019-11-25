#include "VulkanMesh.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "VulkanDescriptorSetLayoutBuilder.h"
#include "VulkanGraphicsPipelineBuilder.h"
#include "VulkanPipelineLayoutBuilder.h"
#include "VulkanRenderPassBuilder.h"

#include "RenderScene.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include <chrono>

/*
 */
void Renderer::init(const RenderScene *scene)
{
	const VulkanShader *pbrVertexShader = scene->getPBRVertexShader();
	const VulkanShader *pbrFragmentShader = scene->getPBRFragmentShader();
	const VulkanShader *skyboxVertexShader = scene->getSkyboxVertexShader();
	const VulkanShader *skyboxFragmentShader = scene->getSkyboxFragmentShader();

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

	VkShaderStageFlags stage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VulkanDescriptorSetLayoutBuilder descriptorSetLayoutBuilder(context);
	descriptorSetLayout = descriptorSetLayoutBuilder
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.build();

	VulkanRenderPassBuilder renderPassBuilder(context);
	renderPass = renderPassBuilder
		.addColorAttachment(swapChainContext.colorFormat, context.maxMSAASamples)
		.addColorResolveAttachment(swapChainContext.colorFormat)
		.addDepthStencilAttachment(swapChainContext.depthFormat, context.maxMSAASamples)
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.addColorResolveAttachmentReference(0, 1)
		.setDepthStencilAttachmentReference(0, 2)
		.build();

	VulkanPipelineLayoutBuilder pipelineLayoutBuilder(context);
	pipelineLayout = pipelineLayoutBuilder
		.addDescriptorSetLayout(descriptorSetLayout)
		.build();

	VulkanGraphicsPipelineBuilder pbrPipelineBuilder(context, pipelineLayout, renderPass);
	pbrPipeline = pbrPipelineBuilder
		.addShaderStage(pbrVertexShader->getShaderModule(), VK_SHADER_STAGE_VERTEX_BIT)
		.addShaderStage(pbrFragmentShader->getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT)
		.addVertexInput(VulkanMesh::getVertexInputBindingDescription(), VulkanMesh::getAttributeDescriptions())
		.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.addViewport(viewport)
		.addScissor(scissor)
		.setRasterizerState(false, false, VK_POLYGON_MODE_FILL, 1.0f, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.setMultisampleState(context.maxMSAASamples, true)
		.setDepthStencilState(true, true, VK_COMPARE_OP_LESS)
		.addBlendColorAttachment()
		.build();

	VulkanGraphicsPipelineBuilder skyboxPipelineBuilder(context, pipelineLayout, renderPass);
	skyboxPipeline = skyboxPipelineBuilder
		.addShaderStage(skyboxVertexShader->getShaderModule(), VK_SHADER_STAGE_VERTEX_BIT)
		.addShaderStage(skyboxFragmentShader->getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT)
		.addVertexInput(VulkanMesh::getVertexInputBindingDescription(), VulkanMesh::getAttributeDescriptions())
		.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.addViewport(viewport)
		.addScissor(scissor)
		.setRasterizerState(false, false, VK_POLYGON_MODE_FILL, 1.0f, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.setMultisampleState(context.maxMSAASamples, true)
		.setDepthStencilState(true, true, VK_COMPARE_OP_LESS)
		.addBlendColorAttachment()
		.build();

	// Create uniform buffers
	VkDeviceSize uboSize = sizeof(RendererState);

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
	descriptorSetAllocInfo.descriptorPool = context.descriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = imageCount;
	descriptorSetAllocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(imageCount);
	if (vkAllocateDescriptorSets(context.device, &descriptorSetAllocInfo, descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Can't allocate descriptor sets");

	initEnvironment(scene);

	for (size_t i = 0; i < imageCount; i++)
	{
		std::array<const VulkanTexture *, 7> textures =
		{
			scene->getAlbedoTexture(),
			scene->getNormalTexture(),
			scene->getAOTexture(),
			scene->getShadingTexture(),
			scene->getEmissionTexture(),
			&environmentCubemap,
			&diffuseIrradianceCubemap,
		};

		VulkanUtils::bindUniformBuffer(
			context,
			descriptorSets[i],
			0,
			uniformBuffers[i],
			0,
			sizeof(RendererState)
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

	// Init ImGui bindings for Vulkan
	{
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = context.instance;
		init_info.PhysicalDevice = context.physicalDevice;
		init_info.Device = context.device;
		init_info.QueueFamily = context.graphicsQueueFamily;
		init_info.Queue = context.graphicsQueue;
		init_info.DescriptorPool = context.descriptorPool;
		init_info.MSAASamples = context.maxMSAASamples;
		init_info.MinImageCount = static_cast<uint32_t>(swapChainContext.swapChainImageViews.size());
		init_info.ImageCount = static_cast<uint32_t>(swapChainContext.swapChainImageViews.size());

		ImGui_ImplVulkan_Init(&init_info, renderPass);

		VulkanRendererContext imGuiContext = {};
		imGuiContext.commandPool = context.commandPool;
		imGuiContext.descriptorPool = context.descriptorPool;
		imGuiContext.device = context.device;
		imGuiContext.graphicsQueue = context.graphicsQueue;
		imGuiContext.maxMSAASamples = context.maxMSAASamples;
		imGuiContext.physicalDevice = context.physicalDevice;
		imGuiContext.presentQueue = context.presentQueue;

		VkCommandBuffer imGuiCommandBuffer = VulkanUtils::beginSingleTimeCommands(imGuiContext);
		ImGui_ImplVulkan_CreateFontsTexture(imGuiCommandBuffer);
		VulkanUtils::endSingleTimeCommands(imGuiContext, imGuiCommandBuffer);
	}
}

void Renderer::initEnvironment(const RenderScene *scene)
{
	environmentCubemap.createCube(VK_FORMAT_R32G32B32A32_SFLOAT, 256, 256, 1);
	diffuseIrradianceCubemap.createCube(VK_FORMAT_R32G32B32A32_SFLOAT, 256, 256, 1);

	setEnvironment(scene, currentEnvironment);
}

void Renderer::setEnvironment(const RenderScene *scene, int index)
{
	{
		VulkanUtils::transitionImageLayout(
			context,
			environmentCubemap.getImage(),
			environmentCubemap.getImageFormat(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			0, environmentCubemap.getNumMipLevels(),
			0, environmentCubemap.getNumLayers()
		);

		hdriToCubeRenderer.init(
			*scene->getCubeVertexShader(),
			*scene->getHDRIToFragmentShader(),
			*scene->getHDRTexture(index),
			environmentCubemap
		);
		hdriToCubeRenderer.render();
		hdriToCubeRenderer.shutdown();

		VulkanUtils::transitionImageLayout(
			context,
			environmentCubemap.getImage(),
			environmentCubemap.getImageFormat(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			0, environmentCubemap.getNumMipLevels(),
			0, environmentCubemap.getNumLayers()
		);
	}

	{
		VulkanUtils::transitionImageLayout(
			context,
			diffuseIrradianceCubemap.getImage(),
			diffuseIrradianceCubemap.getImageFormat(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			0, diffuseIrradianceCubemap.getNumMipLevels(),
			0, diffuseIrradianceCubemap.getNumLayers()
		);

		diffuseIrradianceRenderer.init(
			*scene->getCubeVertexShader(),
			*scene->getDiffuseIrradianceFragmentShader(),
			environmentCubemap,
			diffuseIrradianceCubemap
		);
		diffuseIrradianceRenderer.render();
		diffuseIrradianceRenderer.shutdown();

		VulkanUtils::transitionImageLayout(
			context,
			diffuseIrradianceCubemap.getImage(),
			diffuseIrradianceCubemap.getImageFormat(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			0, diffuseIrradianceCubemap.getNumMipLevels(),
			0, diffuseIrradianceCubemap.getNumLayers()
		);
	}

	std::array<const VulkanTexture *, 2> textures =
	{
		&environmentCubemap,
		&diffuseIrradianceCubemap,
	};

	int imageCount = static_cast<int>(swapChainContext.swapChainImageViews.size());

	for (size_t i = 0; i < imageCount; i++)
		for (int k = 0; k < textures.size(); k++)
			VulkanUtils::bindCombinedImageSampler(
				context,
				descriptorSets[i],
				k + 6,
				textures[k]->getImageView(),
				textures[k]->getSampler()
			);
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

	vkDestroyPipeline(context.device, pbrPipeline, nullptr);
	pbrPipeline = VK_NULL_HANDLE;

	vkDestroyPipeline(context.device, skyboxPipeline, nullptr);
	skyboxPipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(context.device, pipelineLayout, nullptr);
	pipelineLayout = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(context.device, descriptorSetLayout, nullptr);
	descriptorSetLayout = nullptr;

	vkDestroyRenderPass(context.device, renderPass, nullptr);
	renderPass = VK_NULL_HANDLE;

	vkFreeCommandBuffers(context.device, context.commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	commandBuffers.clear();

	vkFreeDescriptorSets(context.device, context.descriptorPool, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data());
	descriptorSets.clear();

	hdriToCubeRenderer.shutdown();
	diffuseIrradianceRenderer.shutdown();

	environmentCubemap.clearGPUData();
	diffuseIrradianceCubemap.clearGPUData();

	ImGui_ImplVulkan_Shutdown();
}

/*
 */
void Renderer::update(const RenderScene *scene)
{
	// Render state
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();

	const float rotationSpeed = 0.1f;
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	const glm::vec3 &up = {0.0f, 0.0f, 1.0f};
	const glm::vec3 &zero = {0.0f, 0.0f, 0.0f};

	const float aspect = swapChainContext.extent.width / (float) swapChainContext.extent.height;
	const float zNear = 0.1f;
	const float zFar = 1000.0f;

	const glm::vec3 &cameraPos = glm::vec3(2.0f, 2.0f, 2.0f);
	const glm::mat4 &rotation = glm::rotate(glm::mat4(1.0f), time * rotationSpeed * glm::radians(90.0f), up);

	state.world = glm::mat4(1.0f);
	state.view = glm::lookAt(cameraPos, zero, up) * rotation;
	state.proj = glm::perspective(glm::radians(60.0f), aspect, zNear, zFar);
	state.proj[1][1] *= -1;
	state.cameraPosWS = glm::vec3(glm::vec4(cameraPos, 1.0f) * rotation);

	// ImGui
	static float f = 0.0f;
	static int counter = 0;
	static bool show_demo_window = false;
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	ImGui::Begin("Material Parameters");

	int oldCurrentEnvironment = currentEnvironment;
	if (ImGui::BeginCombo("Choose Your Destiny", scene->getHDRTexturePath(currentEnvironment)))
	{
		for (int i = 0; i < scene->getNumHDRTextures(); i++)
		{
			bool selected = (i == currentEnvironment);
			if (ImGui::Selectable(scene->getHDRTexturePath(i), &selected))
				currentEnvironment = i;
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::Checkbox("Demo Window", &show_demo_window);

	ImGui::SliderFloat("Lerp User Material", &state.lerpUserValues, 0.0f, 1.0f);
	ImGui::SliderFloat("Metalness", &state.userMetalness, 0.0f, 1.0f);
	ImGui::SliderFloat("Roughness", &state.userRoughness, 0.0f, 1.0f);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();

	if (oldCurrentEnvironment != currentEnvironment)
		setEnvironment(scene, currentEnvironment);
}

VkCommandBuffer Renderer::render(const RenderScene *scene, uint32_t imageIndex)
{
	VkCommandBuffer commandBuffer = commandBuffers[imageIndex];
	VkFramebuffer frameBuffer = frameBuffers[imageIndex];
	VkDescriptorSet descriptorSet = descriptorSets[imageIndex];
	VkBuffer uniformBuffer = uniformBuffers[imageIndex];
	VkDeviceMemory uniformBufferMemory = uniformBuffersMemory[imageIndex];

	void *ubo = nullptr;
	vkMapMemory(context.device, uniformBufferMemory, 0, sizeof(RendererState), 0, &ubo);
	memcpy(ubo, &state, sizeof(RendererState));
	vkUnmapMemory(context.device, uniformBufferMemory);

	if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS)
		throw std::runtime_error("Can't reset command buffer");

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
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapChainContext.extent;

	std::array<VkClearValue, 3> clearValues = {};
	clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
	clearValues[1].color = {0.0f, 0.0f, 0.0f, 1.0f};
	clearValues[2].depthStencil = {1.0f, 0};
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	{
		const VulkanMesh *skybox = scene->getSkybox();

		VkBuffer vertexBuffers[] = { skybox->getVertexBuffer() };
		VkBuffer indexBuffer = skybox->getIndexBuffer();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer, skybox->getNumIndices(), 1, 0, 0, 0);
	}
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pbrPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	{
		const VulkanMesh *mesh = scene->getMesh();

		VkBuffer vertexBuffers[] = { mesh->getVertexBuffer() };
		VkBuffer indexBuffer = mesh->getIndexBuffer();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer, mesh->getNumIndices(), 1, 0, 0, 0);
	}

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Can't record command buffer");
	
	return commandBuffer;
}
