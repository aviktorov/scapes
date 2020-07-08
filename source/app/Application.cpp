#include "Application.h"
#include "ApplicationResources.h"

#include <render/backend/Driver.h>
#include <render/SwapChain.h>

#include "ImGuiRenderer.h"
#include "Renderer.h"
#include "RenderGraph.h"
#include "Scene.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"

#include <iostream>
#include <chrono>

using namespace render;
using namespace render::backend;

/*
 */
void Application::run()
{
	initWindow();
	initImGui();
	initDriver();
	initSwapChain();
	initRenderScene();
	initRenderers();
	mainloop();
	shutdownRenderers();
	shutdownRenderScene();
	shutdownSwapChain();
	shutdownDriver();
	shutdownImGui();
	shutdownWindow();
}

/*
 */
void Application::update()
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();

	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	const glm::vec3 &up = {0.0f, 0.0f, 1.0f};
	const glm::vec3 &zero = {0.0f, 0.0f, 0.0f};

	const float aspect = swap_chain->getWidth() / (float)swap_chain->getHeight();
	const float zNear = 0.1f;
	const float zFar = 100000.0f;

	glm::vec3 cameraPos;
	cameraPos.x = static_cast<float>(glm::cos(camera.phi) * glm::cos(camera.theta) * camera.radius);
	cameraPos.y = static_cast<float>(glm::sin(camera.phi) * glm::cos(camera.theta) * camera.radius);
	cameraPos.z = static_cast<float>(glm::sin(camera.theta) * camera.radius);

	state.world = glm::mat4(1.0f);
	state.view = glm::lookAt(cameraPos, zero, up);
	state.proj = glm::perspective(glm::radians(60.0f), aspect, zNear, zFar);
	state.proj[1][1] *= -1;
	state.cameraPosWS = cameraPos;

	static float f = 0.0f;
	static int counter = 0;

	ImGui::Begin("Material Parameters");

	if (ImGui::Button("Reload Shaders"))
	{
		resources->reloadShaders();
		renderer->setEnvironment(resources, state.currentEnvironment);
	}

	int oldCurrentEnvironment = state.currentEnvironment;
	if (ImGui::BeginCombo("Choose Your Destiny", resources->getHDRTexturePath(state.currentEnvironment)))
	{
		for (int i = 0; i < resources->getNumHDRTextures(); i++)
		{
			bool selected = (i == state.currentEnvironment);
			if (ImGui::Selectable(resources->getHDRTexturePath(i), &selected))
			{
				state.currentEnvironment = i;
				renderer->setEnvironment(resources, i);
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::SliderFloat("Lerp User Material", &state.lerpUserValues, 0.0f, 1.0f);
	ImGui::SliderFloat("Metalness", &state.userMetalness, 0.0f, 1.0f);
	ImGui::SliderFloat("Roughness", &state.userRoughness, 0.0f, 1.0f);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

/*
 */
void Application::render()
{
	RenderFrame frame;
	if (!swap_chain->acquire(frame))
	{
		recreateSwapChain();
		return;
	}

	memcpy(frame.uniform_buffer_data, &state, sizeof(ApplicationState));
	driver->resetCommandBuffer(frame.command_buffer);
	driver->beginCommandBuffer(frame.command_buffer);

	render_graph->render(sponza, frame);

	RenderPassClearValue clear_values[3];
	clear_values[0].color = {0.2f, 0.2f, 0.2f, 1.0f};
	clear_values[1].color = {0.0f, 0.0f, 0.0f, 1.0f};
	clear_values[2].depth_stencil = {1.0f, 0};

	RenderPassLoadOp load_ops[3] = { RenderPassLoadOp::CLEAR, RenderPassLoadOp::DONT_CARE, RenderPassLoadOp::CLEAR };
	RenderPassStoreOp store_ops[3] = { RenderPassStoreOp::STORE, RenderPassStoreOp::STORE, RenderPassStoreOp::DONT_CARE };

	RenderPassInfo info;
	info.load_ops = load_ops;
	info.store_ops = store_ops;
	info.clear_values = clear_values;

	driver->beginRenderPass(frame.command_buffer, frame.frame_buffer, &info);

	renderer->render(resources, frame);
	imgui_renderer->render(frame);

	driver->endRenderPass(frame.command_buffer);

	driver->endCommandBuffer(frame.command_buffer);
	driver->submitSyncked(frame.command_buffer, swap_chain->getBackend());

	if (!swap_chain->present(frame) || windowResized)
	{
		windowResized = false;
		recreateSwapChain();
	}
}

/*
 */
void Application::mainloop()
{
	if (!window)
		return;

	while (!glfwWindowShouldClose(window))
	{
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		update();

		ImGui::Render();

		render();
		glfwPollEvents();
	}

	driver->wait();
}

/*
 */
void Application::initWindow()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(1024, 768, "Vulkan", nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);

	glfwSetFramebufferSizeCallback(window, &Application::onFramebufferResize);
	glfwSetCursorPosCallback(window, &Application::onMousePosition);
	glfwSetMouseButtonCallback(window, &Application::onMouseButton);
	glfwSetScrollCallback(window, &Application::onScroll);
}

void Application::shutdownWindow()
{
	glfwDestroyWindow(window);
	window = nullptr;
}

/*
 */
void Application::onFramebufferResize(GLFWwindow *window, int width, int height)
{
	Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
	assert(application != nullptr);

	application->windowResized = true;
}

void Application::onMousePosition(GLFWwindow* window, double mouseX, double mouseY)
{
	Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
	assert(application != nullptr);

	if (application->input.rotating)
	{
		double deltaX = mouseX - application->input.lastMouseX;
		double deltaY = mouseY - application->input.lastMouseY;

		application->camera.phi -= deltaX * application->input.rotationSpeed;
		application->camera.theta += deltaY * application->input.rotationSpeed;

		application->camera.phi = std::fmod(application->camera.phi, glm::two_pi<double>());
		application->camera.theta = std::clamp<double>(application->camera.theta, -glm::half_pi<double>(), glm::half_pi<double>());
	}

	application->input.lastMouseX = mouseX;
	application->input.lastMouseY = mouseY;
}

void Application::onMouseButton(GLFWwindow* window, int button, int action, int mods)
{
	Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
	assert(application != nullptr);

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
		application->input.rotating = (action == GLFW_PRESS);
}

void Application::onScroll(GLFWwindow* window, double deltaX, double deltaY)
{
	Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
	assert(application);

	application->camera.radius -= deltaY * application->input.scrollSpeed;
}

/*
 */
void Application::initRenderScene()
{
	resources = new ApplicationResources(driver);
	resources->init();

	sponza = new Scene(driver);
	sponza->import("scenes/pbr_sponza/sponza.obj");
}

void Application::shutdownRenderScene()
{
	delete resources;
	resources = nullptr;

	delete sponza;
	sponza = nullptr;
}

/*
 */
void Application::initRenderers()
{
	renderer = new Renderer(driver);
	renderer->init(resources);
	renderer->setEnvironment(resources, state.currentEnvironment);

	imgui_renderer = new ImGuiRenderer(driver, ImGui::GetCurrentContext());
	imgui_renderer->init(swap_chain);

	render_graph = new RenderGraph(driver);
	render_graph->init(resources, swap_chain->getWidth(), swap_chain->getHeight());
}

void Application::shutdownRenderers()
{
	delete renderer;
	renderer = nullptr;

	delete imgui_renderer;
	imgui_renderer = nullptr;

	delete render_graph;
	render_graph = nullptr;
}

/*
 */
void Application::initImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(window, true);
}

void Application::shutdownImGui()
{
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

/*
 */
void Application::initDriver()
{
	driver = backend::Driver::create("PBR Sandbox", "Scape");
}

void Application::shutdownDriver()
{
	delete driver;
	driver = nullptr;
}

/*
 */
void Application::initSwapChain()
{
#if defined(PBR_SANDBOX_WIN32)
	void *nativeWindow = glfwGetWin32Window(window);
#else
	void *nativeWindow = nullptr;
#endif

	if (!swap_chain)
		swap_chain = new render::SwapChain(driver, nativeWindow);

	int width, height;
	glfwGetWindowSize(window, &width, &height);

	uint32_t ubo_size = static_cast<uint32_t>(sizeof(ApplicationState));

	swap_chain->init(static_cast<uint32_t>(width), static_cast<uint32_t>(height), ubo_size);
}

void Application::shutdownSwapChain()
{
	delete swap_chain;
	swap_chain = nullptr;
}

void Application::recreateSwapChain()
{
	int width = 0, height = 0;
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}
	driver->wait();

	glfwGetWindowSize(window, &width, &height);

	swap_chain->resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	render_graph->resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	imgui_renderer->resize(swap_chain);
}
