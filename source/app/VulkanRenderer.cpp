#include "VulkanRenderer.h"
#include "VulkanMesh.h"
#include "VulkanSwapChain.h"

#include "RenderScene.h"

#include "render/backend/vulkan/driver.h"
#include "render/backend/vulkan/device.h"
#include "render/backend/vulkan/VulkanUtils.h"
#include "render/backend/vulkan/VulkanDescriptorSetLayoutBuilder.h"
#include "render/backend/vulkan/VulkanGraphicsPipelineBuilder.h"
#include "render/backend/vulkan/VulkanPipelineLayoutBuilder.h"
#include "render/backend/vulkan/VulkanRenderPassBuilder.h"

using namespace render::backend;

/*
 */
VulkanRenderer::VulkanRenderer(
	render::backend::Driver *driver,
	VkExtent2D extent,
	VkDescriptorSetLayout descriptorSetLayout,
	VkRenderPass renderPass
)
	: driver(driver)
	, extent(extent)
	, renderPass(renderPass)
	, descriptorSetLayout(descriptorSetLayout)
	, hdriToCubeRenderer(driver)
	, diffuseIrradianceRenderer(driver)
	, environmentCubemap(driver)
	, diffuseIrradianceCubemap(driver)
	, bakedBRDF(driver)
	, bakedBRDFRenderer(driver)
{
	device = static_cast<render::backend::VulkanDriver *>(driver)->getDevice();
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

	VulkanDescriptorSetLayoutBuilder sceneDescriptorSetLayoutBuilder;
	sceneDescriptorSetLayout = sceneDescriptorSetLayoutBuilder
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, 0)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, 1)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, 2)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, 3)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, 4)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, 5)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, 6)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, 7)
		.build(device->getDevice());

	VulkanPipelineLayoutBuilder pipelineLayoutBuilder;
	pipelineLayout = pipelineLayoutBuilder
		.addDescriptorSetLayout(descriptorSetLayout)
		.addDescriptorSetLayout(sceneDescriptorSetLayout)
		.build(device->getDevice());

	VulkanGraphicsPipelineBuilder pbrPipelineBuilder(pipelineLayout, renderPass);
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
		.setMultisampleState(device->getMaxSampleCount(), true)
		.setDepthStencilState(true, true, VK_COMPARE_OP_LESS)
		.addBlendColorAttachment()
		.build(device->getDevice());

	VulkanGraphicsPipelineBuilder skyboxPipelineBuilder(pipelineLayout, renderPass);
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
		.setMultisampleState(device->getMaxSampleCount(), true)
		.setDepthStencilState(true, true, VK_COMPARE_OP_LESS)
		.addBlendColorAttachment()
		.build(device->getDevice());

	bakedBRDF.create2D(render::backend::Format::R16G16_SFLOAT, 256, 256, 1);
	environmentCubemap.createCube(render::backend::Format::R32G32B32A32_SFLOAT, 256, 256, 8);
	diffuseIrradianceCubemap.createCube(render::backend::Format::R32G32B32A32_SFLOAT, 256, 256, 1);

	bakedBRDFRenderer.init(
		*scene->getBakedBRDFVertexShader(),
		*scene->getBakedBRDFFragmentShader(),
		bakedBRDF
	);

	{
		VulkanUtils::transitionImageLayout(
			device,
			bakedBRDF.getImage(),
			bakedBRDF.getImageFormat(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			0, bakedBRDF.getNumMipLevels(),
			0, bakedBRDF.getNumLayers()
		);

		bakedBRDFRenderer.render();

		VulkanUtils::transitionImageLayout(
			device,
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
		VulkanCubemapRenderer *mipRenderer = new VulkanCubemapRenderer(driver);
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
	sceneDescriptorSetAllocInfo.descriptorPool = device->getDescriptorPool();
	sceneDescriptorSetAllocInfo.descriptorSetCount = 1;
	sceneDescriptorSetAllocInfo.pSetLayouts = &sceneDescriptorSetLayout;

	if (vkAllocateDescriptorSets(device->getDevice(), &sceneDescriptorSetAllocInfo, &sceneDescriptorSet) != VK_SUCCESS)
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
			device,
			sceneDescriptorSet,
			k,
			textures[k]->getImageView(),
			textures[k]->getSampler()
		);
}

void VulkanRenderer::shutdown()
{
	vkDestroyPipeline(device->getDevice(), pbrPipeline, nullptr);
	pbrPipeline = VK_NULL_HANDLE;

	vkDestroyPipeline(device->getDevice(), skyboxPipeline, nullptr);
	skyboxPipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(device->getDevice(), pipelineLayout, nullptr);
	pipelineLayout = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(device->getDevice(), sceneDescriptorSetLayout, nullptr);
	sceneDescriptorSetLayout = VK_NULL_HANDLE;

	vkFreeDescriptorSets(device->getDevice(), device->getDescriptorPool(), 1, &sceneDescriptorSet);
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
	VkCommandBuffer command_buffer = static_cast<vulkan::CommandBuffer *>(frame.command_buffer)->command_buffer;
	VkDescriptorSet descriptor_set = frame.descriptor_set;

	std::array<VkDescriptorSet, 2> sets = {descriptor_set, sceneDescriptorSet};

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

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);
	{
		const VulkanMesh *skybox = scene->getSkybox();

		VkBuffer vertexBuffers[] = { skybox->getVertexBuffer() };
		VkBuffer indexBuffer = skybox->getIndexBuffer();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(command_buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(command_buffer, skybox->getNumIndices(), 1, 0, 0, 0);
	}

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pbrPipeline);
	{
		const VulkanMesh *mesh = scene->getMesh();

		VkBuffer vertexBuffers[] = { mesh->getVertexBuffer() };
		VkBuffer indexBuffer = mesh->getIndexBuffer();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(command_buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(command_buffer, mesh->getNumIndices(), 1, 0, 0, 0);
	}
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
			device,
			environmentCubemap.getImage(),
			environmentCubemap.getImageFormat(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			0, 1,
			0, environmentCubemap.getNumLayers()
		);

		hdriToCubeRenderer.render(*texture);

		VulkanUtils::transitionImageLayout(
			device,
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
			device,
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
			device,
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
			device,
			diffuseIrradianceCubemap.getImage(),
			diffuseIrradianceCubemap.getImageFormat(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			0, diffuseIrradianceCubemap.getNumMipLevels(),
			0, diffuseIrradianceCubemap.getNumLayers()
		);

		diffuseIrradianceRenderer.render(environmentCubemap);

		VulkanUtils::transitionImageLayout(
			device,
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
			device,
			sceneDescriptorSet,
			k + 5,
			textures[k]->getImageView(),
			textures[k]->getSampler()
		);
}
