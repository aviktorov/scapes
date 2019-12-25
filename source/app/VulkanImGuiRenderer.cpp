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
void VulkanImGuiRenderer::init(const RendererState *state, const RenderScene *scene, const VulkanSwapChain *swapChain)
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

void VulkanImGuiRenderer::resize(const VulkanSwapChain *swapChain)
{
	extent = swapChain->getExtent();
	ImGui_ImplVulkan_SetMinImageCount(swapChain->getNumImages());
}

/*
 */
void VulkanImGuiRenderer::update(RendererState *state, const RenderScene *scene)
{
	static float f = 0.0f;
	static int counter = 0;
	static bool show_demo_window = false;
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	ImGui::Begin("Material Parameters");

	int oldCurrentEnvironment = state->currentEnvironment;
	if (ImGui::BeginCombo("Choose Your Destiny", scene->getHDRTexturePath(state->currentEnvironment)))
	{
		for (int i = 0; i < scene->getNumHDRTextures(); i++)
		{
			bool selected = (i == state->currentEnvironment);
			if (ImGui::Selectable(scene->getHDRTexturePath(i), &selected))
				state->currentEnvironment = i;
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::Checkbox("Demo Window", &show_demo_window);

	ImGui::SliderFloat("Lerp User Material", &state->lerpUserValues, 0.0f, 1.0f);
	ImGui::SliderFloat("Metalness", &state->userMetalness, 0.0f, 1.0f);
	ImGui::SliderFloat("Roughness", &state->userRoughness, 0.0f, 1.0f);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

void VulkanImGuiRenderer::render(const RendererState *state, const RenderScene *scene, const VulkanRenderFrame &frame)
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
