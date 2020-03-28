#include "VulkanRenderer.h"
#include "VulkanContext.h"
#include "VulkanMesh.h"
#include "VulkanUtils.h"
#include "VulkanDescriptorSetLayoutBuilder.h"
#include "VulkanGraphicsPipelineBuilder.h"
#include "VulkanPipelineLayoutBuilder.h"
#include "VulkanRenderPassBuilder.h"
#include "VulkanSwapChain.h"

#include "RenderScene.h"

/*
 */
VulkanRenderer::VulkanRenderer(
	const VulkanContext *context,
	VkExtent2D extent,
	VkDescriptorSetLayout descriptorSetLayout,
	VkRenderPass renderPass
)
	: context(context)
	, extent(extent)
	, renderPass(renderPass)
	, descriptorSetLayout(descriptorSetLayout)
	, hdriToCubeRenderer(context)
	, diffuseIrradianceRenderer(context)
	, environmentCubemap(context)
	, diffuseIrradianceCubemap(context)
	, bakedBRDF(context)
	, bakedBRDFRenderer(context)
{
}

VulkanRenderer::~VulkanRenderer()
{
	shutdown();
}

/*
 */
void VulkanRenderer::init(const RenderScene *scene)
{
	const VulkanShader *pbrVertexShader = scene->getPBRVertexShader();
	const VulkanShader *pbrFragmentShader = scene->getPBRFragmentShader();
	const VulkanShader *skyboxVertexShader = scene->getSkyboxVertexShader();
	const VulkanShader *skyboxFragmentShader = scene->getSkyboxFragmentShader();

	VkShaderStageFlags stage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VulkanDescriptorSetLayoutBuilder sceneDescriptorSetLayoutBuilder(context);
	sceneDescriptorSetLayout = sceneDescriptorSetLayoutBuilder
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage)
		.build();

	VulkanPipelineLayoutBuilder pipelineLayoutBuilder(context);
	pipelineLayout = pipelineLayoutBuilder
		.addDescriptorSetLayout(descriptorSetLayout)
		.addDescriptorSetLayout(sceneDescriptorSetLayout)
		.build();

	VulkanGraphicsPipelineBuilder pbrPipelineBuilder(context, pipelineLayout, renderPass);
	pbrPipeline = pbrPipelineBuilder
		.addShaderStage(pbrVertexShader->getShaderModule(), VK_SHADER_STAGE_VERTEX_BIT)
		.addShaderStage(pbrFragmentShader->getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT)
		.addVertexInput(VulkanMesh::getVertexInputBindingDescription(), VulkanMesh::getAttributeDescriptions())
		.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.addViewport(VkViewport())
		.addScissor(VkRect2D())
		.addDynamicState(VK_DYNAMIC_STATE_SCISSOR)
		.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
		.setRasterizerState(false, false, VK_POLYGON_MODE_FILL, 1.0f, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.setMultisampleState(context->getMaxMSAASamples(), true)
		.setDepthStencilState(true, true, VK_COMPARE_OP_LESS)
		.addBlendColorAttachment()
		.build();

	VulkanGraphicsPipelineBuilder skyboxPipelineBuilder(context, pipelineLayout, renderPass);
	skyboxPipeline = skyboxPipelineBuilder
		.addShaderStage(skyboxVertexShader->getShaderModule(), VK_SHADER_STAGE_VERTEX_BIT)
		.addShaderStage(skyboxFragmentShader->getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT)
		.addVertexInput(VulkanMesh::getVertexInputBindingDescription(), VulkanMesh::getAttributeDescriptions())
		.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.addViewport(VkViewport())
		.addScissor(VkRect2D())
		.addDynamicState(VK_DYNAMIC_STATE_SCISSOR)
		.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
		.setRasterizerState(false, false, VK_POLYGON_MODE_FILL, 1.0f, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.setMultisampleState(context->getMaxMSAASamples(), true)
		.setDepthStencilState(true, true, VK_COMPARE_OP_LESS)
		.addBlendColorAttachment()
		.build();

	bakedBRDF.create2D(VK_FORMAT_R16G16_SFLOAT, 256, 256, 1);
	environmentCubemap.createCube(VK_FORMAT_R32G32B32A32_SFLOAT, 256, 256, 8);
	diffuseIrradianceCubemap.createCube(VK_FORMAT_R32G32B32A32_SFLOAT, 256, 256, 1);

	bakedBRDFRenderer.init(
		*scene->getBakedBRDFVertexShader(),
		*scene->getBakedBRDFFragmentShader(),
		bakedBRDF
	);

	{
		VulkanUtils::transitionImageLayout(
			context,
			bakedBRDF.getImage(),
			bakedBRDF.getImageFormat(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			0, bakedBRDF.getNumMipLevels(),
			0, bakedBRDF.getNumLayers()
		);

		bakedBRDFRenderer.render();

		VulkanUtils::transitionImageLayout(
			context,
			bakedBRDF.getImage(),
			bakedBRDF.getImageFormat(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			0, bakedBRDF.getNumMipLevels(),
			0, bakedBRDF.getNumLayers()
		);
	}

	hdriToCubeRenderer.init(
		*scene->getCubeVertexShader(),
		*scene->getHDRIToFragmentShader(),
		environmentCubemap,
		0
	);

	cubeToPrefilteredRenderers.resize(environmentCubemap.getNumMipLevels() - 1);
	for (int mip = 0; mip < environmentCubemap.getNumMipLevels() - 1; mip++)
	{
		VulkanCubemapRenderer *mipRenderer = new VulkanCubemapRenderer(context);
		mipRenderer->init(
			*scene->getCubeVertexShader(),
			*scene->getCubeToPrefilteredSpecularShader(),
			environmentCubemap,
			mip + 1,
			sizeof(float) * 4
		);

		cubeToPrefilteredRenderers[mip] = mipRenderer;
	}

	diffuseIrradianceRenderer.init(
		*scene->getCubeVertexShader(),
		*scene->getDiffuseIrradianceFragmentShader(),
		diffuseIrradianceCubemap,
		0
	);

	// Create scene descriptor set
	VkDescriptorSetAllocateInfo sceneDescriptorSetAllocInfo = {};
	sceneDescriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	sceneDescriptorSetAllocInfo.descriptorPool = context->getDescriptorPool();
	sceneDescriptorSetAllocInfo.descriptorSetCount = 1;
	sceneDescriptorSetAllocInfo.pSetLayouts = &sceneDescriptorSetLayout;

	if (vkAllocateDescriptorSets(context->getDevice(), &sceneDescriptorSetAllocInfo, &sceneDescriptorSet) != VK_SUCCESS)
		throw std::runtime_error("Can't allocate scene descriptor set");

	std::array<const VulkanTexture *, 8> textures =
	{
		scene->getAlbedoTexture(),
		scene->getNormalTexture(),
		scene->getAOTexture(),
		scene->getShadingTexture(),
		scene->getEmissionTexture(),
		&environmentCubemap,
		&diffuseIrradianceCubemap,
		&bakedBRDF
	};

	for (int k = 0; k < textures.size(); k++)
		VulkanUtils::bindCombinedImageSampler(
			context,
			sceneDescriptorSet,
			k,
			textures[k]->getImageView(),
			textures[k]->getSampler()
		);
}

void VulkanRenderer::shutdown()
{
	vkDestroyPipeline(context->getDevice(), pbrPipeline, nullptr);
	pbrPipeline = VK_NULL_HANDLE;

	vkDestroyPipeline(context->getDevice(), skyboxPipeline, nullptr);
	skyboxPipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(context->getDevice(), pipelineLayout, nullptr);
	pipelineLayout = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(context->getDevice(), sceneDescriptorSetLayout, nullptr);
	sceneDescriptorSetLayout = VK_NULL_HANDLE;

	vkFreeDescriptorSets(context->getDevice(), context->getDescriptorPool(), 1, &sceneDescriptorSet);
	sceneDescriptorSet = VK_NULL_HANDLE;

	for (VulkanCubemapRenderer *renderer : cubeToPrefilteredRenderers)
	{
		renderer->shutdown();
		delete renderer;
	}
	cubeToPrefilteredRenderers.clear();

	bakedBRDFRenderer.shutdown();
	hdriToCubeRenderer.shutdown();
	diffuseIrradianceRenderer.shutdown();

	bakedBRDF.clearGPUData();
	environmentCubemap.clearGPUData();
	diffuseIrradianceCubemap.clearGPUData();
}

/*
 */
void VulkanRenderer::resize(const VulkanSwapChain *swapChain)
{
	extent = swapChain->getExtent();
}

void VulkanRenderer::render(const RenderScene *scene, const VulkanRenderFrame &frame)
{
	VkCommandBuffer commandBuffer = frame.commandBuffer;
	VkFramebuffer frameBuffer = frame.frameBuffer;
	VkDescriptorSet descriptorSet = frame.descriptorSet;

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = frameBuffer;
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = extent;

	std::array<VkClearValue, 3> clearValues = {};
	clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
	clearValues[1].color = {0.0f, 0.0f, 0.0f, 1.0f};
	clearValues[2].depthStencil = {1.0f, 0};
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	std::array<VkDescriptorSet, 2> sets = {descriptorSet, sceneDescriptorSet};

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = extent;

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);
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
	{
		const VulkanMesh *mesh = scene->getMesh();

		VkBuffer vertexBuffers[] = { mesh->getVertexBuffer() };
		VkBuffer indexBuffer = mesh->getIndexBuffer();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer, mesh->getNumIndices(), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(commandBuffer);
}

/*
 */
void VulkanRenderer::reload(const RenderScene *scene)
{
	shutdown();
	init(scene);
}

void VulkanRenderer::setEnvironment(const VulkanTexture *texture)
{
	{
		VulkanUtils::transitionImageLayout(
			context,
			environmentCubemap.getImage(),
			environmentCubemap.getImageFormat(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			0, 1,
			0, environmentCubemap.getNumLayers()
		);

		hdriToCubeRenderer.render(*texture);

		VulkanUtils::transitionImageLayout(
			context,
			environmentCubemap.getImage(),
			environmentCubemap.getImageFormat(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			0, 1,
			0, environmentCubemap.getNumLayers()
		);
	}

	for (uint32_t i = 0; i < cubeToPrefilteredRenderers.size(); i++)
	{
		VulkanUtils::transitionImageLayout(
			context,
			environmentCubemap.getImage(),
			environmentCubemap.getImageFormat(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			i + 1, 1,
			0, environmentCubemap.getNumLayers()
		);

		float data[4] = {
			static_cast<float>(i) / environmentCubemap.getNumMipLevels(),
			0.0f, 0.0f, 0.0f
		};

		cubeToPrefilteredRenderers[i]->render(environmentCubemap, data, i);

		VulkanUtils::transitionImageLayout(
			context,
			environmentCubemap.getImage(),
			environmentCubemap.getImageFormat(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			i + 1, 1,
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

		diffuseIrradianceRenderer.render(environmentCubemap);

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

	for (int k = 0; k < textures.size(); k++)
		VulkanUtils::bindCombinedImageSampler(
			context,
			sceneDescriptorSet,
			k + 5,
			textures[k]->getImageView(),
			textures[k]->getSampler()
		);
}
