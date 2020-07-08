// TODO: remove Vulkan dependencies
#include <volk.h>
#include <render/backend/vulkan/Driver.h>
#include <render/backend/vulkan/Device.h>
#include <render/backend/vulkan/Utils.h>

#include "ImGuiRenderer.h"

#include <render/SwapChain.h>
#include <render/Texture.h>

#include "imgui.h"
#include "imgui_impl_vulkan.h"

using namespace render;
using namespace render::backend;

/*
 */
ImGuiRenderer::ImGuiRenderer(
	Driver *driver,
	ImGuiContext *imguiContext
)
	: driver(driver)
	, imguiContext(imguiContext)
{
	ImGui::SetCurrentContext(imguiContext);
}

ImGuiRenderer::~ImGuiRenderer()
{
	shutdown();
}

/*
 */
void *ImGuiRenderer::addTexture(const render::Texture *texture) const
{
	const vulkan::Texture *vk_texture = static_cast<const vulkan::Texture *>(texture->getBackend());

	return ImGui_ImplVulkan_AddTexture(vk_texture->sampler, vk_texture->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

/*
 */
void ImGuiRenderer::init(const render::SwapChain *swap_chain)
{
	const vulkan::Device *vk_device = static_cast<VulkanDriver *>(driver)->getDevice();
	const vulkan::SwapChain *vk_swap_chain = static_cast<const vulkan::SwapChain *>(swap_chain->getBackend());
	const vulkan::FrameBuffer *vk_frame_buffer = static_cast<const vulkan::FrameBuffer *>(swap_chain->getFrameBuffer(0));

	// Init ImGui bindings for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vk_device->getInstance();
	init_info.PhysicalDevice = vk_device->getPhysicalDevice();
	init_info.Device = vk_device->getDevice();
	init_info.QueueFamily = vk_device->getGraphicsQueueFamily();
	init_info.Queue = vk_device->getGraphicsQueue();
	init_info.DescriptorPool = vk_device->getDescriptorPool();
	init_info.MSAASamples = vk_device->getMaxSampleCount();
	init_info.MinImageCount = vk_swap_chain->num_images;
	init_info.ImageCount = vk_swap_chain->num_images;

	ImGui_ImplVulkan_Init(&init_info, vk_frame_buffer->dummy_render_pass);

	VkCommandBuffer temp_command_buffer = vulkan::Utils::beginSingleTimeCommands(vk_device);
	ImGui_ImplVulkan_CreateFontsTexture(temp_command_buffer);
	vulkan::Utils::endSingleTimeCommands(vk_device, temp_command_buffer);
}

void ImGuiRenderer::shutdown()
{
	ImGui_ImplVulkan_Shutdown();
	imguiContext = nullptr;
}

/*
 */
void ImGuiRenderer::resize(const render::SwapChain *swap_chain)
{
	uint32_t num_images = driver->getNumSwapChainImages(swap_chain->getBackend());
	ImGui_ImplVulkan_SetMinImageCount(num_images);
}

void ImGuiRenderer::render(const render::RenderFrame &frame)
{
	VkCommandBuffer command_buffer = static_cast<vulkan::CommandBuffer *>(frame.command_buffer)->command_buffer;
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
}
