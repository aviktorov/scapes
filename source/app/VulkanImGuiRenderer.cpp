#include "VulkanApplication.h"
#include "VulkanImGuiRenderer.h"
#include "VulkanSwapChain.h"
#include "VulkanUtils.h"

#include "RenderScene.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include <array>

/*
 */
VulkanImGuiRenderer::VulkanImGuiRenderer(
	const VulkanRendererContext &context,
	VkExtent2D extent,
	VkRenderPass renderPass
)
	: context(context)
	, extent(extent)
	, renderPass(renderPass)
{
}

VulkanImGuiRenderer::~VulkanImGuiRenderer()
{
	shutdown();
}

/*
 */
void VulkanImGuiRenderer::init(const RenderScene *scene, const VulkanSwapChain *swapChain)
{
	// Init ImGui bindings for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = context.instance;
	init_info.PhysicalDevice = context.physicalDevice;
	init_info.Device = context.device;
	init_info.QueueFamily = context.graphicsQueueFamily;
	init_info.Queue = context.graphicsQueue;
	init_info.DescriptorPool = context.descriptorPool;
	init_info.MSAASamples = context.maxMSAASamples;
	init_info.MinImageCount = static_cast<uint32_t>(swapChain->getNumImages());
	init_info.ImageCount = static_cast<uint32_t>(swapChain->getNumImages());

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

void VulkanImGuiRenderer::shutdown()
{
	ImGui_ImplVulkan_Shutdown();
}

/*
 */
void VulkanImGuiRenderer::resize(const VulkanSwapChain *swapChain)
{
	extent = swapChain->getExtent();
	ImGui_ImplVulkan_SetMinImageCount(swapChain->getNumImages());
}

void VulkanImGuiRenderer::render(const RenderScene *scene, const VulkanRenderFrame &frame)
{
	VkCommandBuffer commandBuffer = frame.commandBuffer;
	VkFramebuffer frameBuffer = frame.frameBuffer;
	VkDeviceMemory uniformBufferMemory = frame.uniformBufferMemory;
	VkDescriptorSet descriptorSet = frame.descriptorSet;

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = frameBuffer;
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = extent;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame.commandBuffer);

	vkCmdEndRenderPass(commandBuffer);
}
