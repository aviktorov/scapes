// TODO: remove Vulkan dependencies
#include "VulkanCubemapRenderer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

#include "render/backend/vulkan/driver.h"
#include "render/backend/vulkan/device.h"
#include "render/backend/vulkan/VulkanUtils.h"
#include "render/backend/vulkan/VulkanDescriptorSetLayoutBuilder.h"
#include "render/backend/vulkan/VulkanGraphicsPipelineBuilder.h"
#include "render/backend/vulkan/VulkanPipelineLayoutBuilder.h"
#include "render/backend/vulkan/VulkanRenderPassBuilder.h"

/*
 */
struct CubemapFaceOrientationData
{
	glm::mat4 faces[6];
};

/*
 */
VulkanCubemapRenderer::VulkanCubemapRenderer(render::backend::Driver *driver)
	: driver(driver)
	, quad(driver)
{
	device = static_cast<render::backend::VulkanDriver *>(driver)->getDevice();
}

/*
 */
void VulkanCubemapRenderer::init(
	const VulkanShader &vertex_shader,
	const VulkanShader &fragment_shader,
	const VulkanTexture &target_texture,
	int mip,
	uint32_t push_constants_size_
)
{
	push_constants_size = push_constants_size_;
	quad.createQuad(2.0f);

	target_extent.width = target_texture.getWidth(mip);
	target_extent.height = target_texture.getHeight(mip);

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

	VulkanDescriptorSetLayoutBuilder descriptor_set_layout_builder;
	descriptor_set_layout = descriptor_set_layout_builder
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage, 0)
		.addDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, 1)
		.build(device->getDevice());

	VulkanRenderPassBuilder render_pass_builder;
	render_pass = render_pass_builder
		.addColorAttachment(target_texture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorAttachment(target_texture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorAttachment(target_texture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorAttachment(target_texture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorAttachment(target_texture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addColorAttachment(target_texture.getImageFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
		.addColorAttachmentReference(0, 0)
		.addColorAttachmentReference(0, 1)
		.addColorAttachmentReference(0, 2)
		.addColorAttachmentReference(0, 3)
		.addColorAttachmentReference(0, 4)
		.addColorAttachmentReference(0, 5)
		.build(device->getDevice());

	VulkanPipelineLayoutBuilder pipeline_layout_builder;
	pipeline_layout_builder.addDescriptorSetLayout(descriptor_set_layout);

	if (push_constants_size > 0)
		pipeline_layout_builder.addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, push_constants_size);

	pipeline_layout = pipeline_layout_builder.build(device->getDevice());

	VulkanGraphicsPipelineBuilder pipelineBuilder(pipeline_layout, render_pass);
	pipeline = pipelineBuilder
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
		.addBlendColorAttachment()
		.addBlendColorAttachment()
		.addBlendColorAttachment()
		.addBlendColorAttachment()
		.addBlendColorAttachment()
		.build(device->getDevice());

	// Create uniform buffers
	uint32_t ubo_size = sizeof(CubemapFaceOrientationData);
	uniform_buffer = driver->createUniformBuffer(render::backend::BufferType::DYNAMIC, ubo_size);

	// Create descriptor set
	VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {};
	descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_alloc_info.descriptorPool = device->getDescriptorPool();
	descriptor_set_alloc_info.descriptorSetCount = 1;
	descriptor_set_alloc_info.pSetLayouts = &descriptor_set_layout;

	if (vkAllocateDescriptorSets(device->getDevice(), &descriptor_set_alloc_info, &descriptor_set) != VK_SUCCESS)
		throw std::runtime_error("Can't allocate descriptor sets");

	// Create framebuffer
	render::backend::FrameBufferAttachmentType type = render::backend::FrameBufferAttachmentType::COLOR;
	render::backend::FrameBufferAttachment attachments[6] =
	{
		{ type, target_texture.getBackend(), static_cast<uint32_t>(mip), 1, 0, 1 },
		{ type, target_texture.getBackend(), static_cast<uint32_t>(mip), 1, 1, 1 },
		{ type, target_texture.getBackend(), static_cast<uint32_t>(mip), 1, 2, 1 },
		{ type, target_texture.getBackend(), static_cast<uint32_t>(mip), 1, 3, 1 },
		{ type, target_texture.getBackend(), static_cast<uint32_t>(mip), 1, 4, 1 },
		{ type, target_texture.getBackend(), static_cast<uint32_t>(mip), 1, 5, 1 },
	};

	framebuffer = driver->createFrameBuffer(6, attachments);

	// Create command buffer
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool = device->getCommandPool();
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device->getDevice(), &allocate_info, &command_buffer) != VK_SUCCESS)
		throw std::runtime_error("Can't create command buffers");

	// Fill uniform buffer
	CubemapFaceOrientationData *ubo = reinterpret_cast<CubemapFaceOrientationData *>(driver->map(uniform_buffer));

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

	driver->unmap(uniform_buffer);

	// Bind data to descriptor set
	VulkanUtils::bindUniformBuffer(
		device,
		descriptor_set,
		0,
		static_cast<render::backend::vulkan::UniformBuffer *>(uniform_buffer)->buffer,
		0,
		ubo_size
	);

	// Create fence
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;

	if (vkCreateFence(device->getDevice(), &fenceInfo, nullptr, &fence) != VK_SUCCESS)
		throw std::runtime_error("Can't create fence");
}

void VulkanCubemapRenderer::shutdown()
{
	driver->destroyUniformBuffer(uniform_buffer);
	uniform_buffer = nullptr;

	driver->destroyFrameBuffer(framebuffer);
	framebuffer = nullptr;

	vkDestroyPipeline(device->getDevice(), pipeline, nullptr);
	pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(device->getDevice(), pipeline_layout, nullptr);
	pipeline_layout = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(device->getDevice(), descriptor_set_layout, nullptr);
	descriptor_set_layout = VK_NULL_HANDLE;

	vkDestroyRenderPass(device->getDevice(), render_pass, nullptr);
	render_pass = VK_NULL_HANDLE;

	vkFreeCommandBuffers(device->getDevice(), device->getCommandPool(), 1, &command_buffer);
	command_buffer = VK_NULL_HANDLE;

	vkFreeDescriptorSets(device->getDevice(), device->getDescriptorPool(), 1, &descriptor_set);
	descriptor_set = VK_NULL_HANDLE;

	vkDestroyFence(device->getDevice(), fence, nullptr);
	fence = VK_NULL_HANDLE;

	quad.clearGPUData();
	quad.clearCPUData();
}

/*
 */
void VulkanCubemapRenderer::render(const VulkanTexture &input_texture, float *push_constants, int input_mip)
{
	VkImageView mip_view = (input_mip == -1) ? VK_NULL_HANDLE : VulkanUtils::createImageView(
		device,
		input_texture.getImage(),
		input_texture.getImageFormat(),
		VK_IMAGE_ASPECT_COLOR_BIT,
		(input_texture.getNumLayers() == 6) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
		input_mip, 1, 0, input_texture.getNumLayers()
	);

	VulkanUtils::bindCombinedImageSampler(
		device,
		descriptor_set,
		1,
		(input_mip == -1) ? input_texture.getImageView() : mip_view,
		input_texture.getSampler()
	);

	// Record command buffer
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("Can't begin recording command buffer");

	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = render_pass;
	render_pass_info.framebuffer = static_cast<render::backend::vulkan::FrameBuffer *>(framebuffer)->framebuffer;
	render_pass_info.renderArea.offset = { 0, 0 };
	render_pass_info.renderArea.extent.width = target_extent.width;
	render_pass_info.renderArea.extent.height = target_extent.height;

	VkClearValue clear_values[6];
	for (int i = 0; i < 6; i++)
	{
		clear_values[i] = {};
		clear_values[i].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	}
	render_pass_info.clearValueCount = 6;
	render_pass_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

	if (push_constants_size > 0 && push_constants)
		vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, push_constants_size, push_constants);

	{
		VkBuffer vertex_buffers[] = { quad.getVertexBuffer() };
		VkBuffer index_buffer = quad.getIndexBuffer();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
		vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(command_buffer, quad.getNumIndices(), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(command_buffer);

	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
		throw std::runtime_error("Can't record command buffer");

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	if (vkResetFences(device->getDevice(), 1, &fence) != VK_SUCCESS)
		throw std::runtime_error("Can't reset fence");

	if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submit_info, fence) != VK_SUCCESS)
		throw std::runtime_error("Can't submit command buffer");

	if (vkWaitForFences(device->getDevice(), 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
		throw std::runtime_error("Can't wait for a fence");
	
	vkDestroyImageView(device->getDevice(), mip_view, nullptr);
}
