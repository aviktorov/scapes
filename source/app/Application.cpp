#if defined(SCAPES_PLATFORM_WIN32)
	#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include "Application.h"
#include "ApplicationResources.h"

#include <render/shaders/spirv/Compiler.h>
#include <render/backend/Driver.h>
#include <render/SwapChain.h>

#include "SkyLight.h"
#include "Renderer.h"
#include "RenderGraph.h"
#include "Scene.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"

#include <algorithm>
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
	const float zFar = 10000.0f;

	glm::vec3 cameraPos;
	cameraPos.x = static_cast<float>(glm::cos(camera.phi) * glm::cos(camera.theta) * camera.radius);
	cameraPos.y = static_cast<float>(glm::sin(camera.phi) * glm::cos(camera.theta) * camera.radius);
	cameraPos.z = static_cast<float>(glm::sin(camera.theta) * camera.radius);

	glm::vec4 cameraParams;
	cameraParams.x = zNear;
	cameraParams.y = zFar;
	cameraParams.z = 1.0f / zNear;
	cameraParams.w = 1.0f / zFar;

	state.world = glm::mat4(1.0f);
	state.view = glm::lookAt(cameraPos, zero, up);
	state.invView = glm::inverse(state.view);
	state.proj = glm::perspective(glm::radians(60.0f), aspect, zNear, zFar);
	state.proj[1][1] *= -1;
	state.invProj = glm::inverse(state.proj);
	state.cameraPosWS = cameraPos;
	state.cameraParams = cameraParams;
	state.currentTime = time;

	static float f = 0.0f;
	static int counter = 0;

	ImGui::Begin("Material Parameters");

	if (ImGui::Button("Reload Shaders"))
	{
		resources->reloadShaders();
		sky_light->setEnvironmentCubemap(resources->getHDREnvironmentCubemap(state.currentEnvironment));
		sky_light->setIrradianceCubemap(resources->getHDRIrradianceCubemap(state.currentEnvironment));
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
				sky_light->setEnvironmentCubemap(resources->getHDREnvironmentCubemap(state.currentEnvironment));
				sky_light->setIrradianceCubemap(resources->getHDRIrradianceCubemap(state.currentEnvironment));
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::SliderFloat("Lerp User Material", &state.lerpUserValues, 0.0f, 1.0f);
	ImGui::SliderFloat("Metalness", &state.userMetalness, 0.0f, 1.0f);
	ImGui::SliderFloat("Roughness", &state.userRoughness, 0.0f, 1.0f);

	ImGui::SliderFloat("Radius", &render_graph->getSSAOKernel().cpu_data->radius, 0.0f, 100.0f);
	ImGui::SliderFloat("Intensity", &render_graph->getSSAOKernel().cpu_data->intensity, 0.0f, 100.0f);
	if (ImGui::SliderInt("Samples", (int*)&render_graph->getSSAOKernel().cpu_data->num_samples, 32, 256))
	{
		render_graph->buildSSAOKernel();
	}

	ImGui::SliderFloat("SSR Coarse Step Size", &render_graph->getSSRData().cpu_data->coarse_step_size, 0.0f, 200.0f);
	ImGui::SliderInt("SSR Num Coarse Steps", (int*)&render_graph->getSSRData().cpu_data->num_coarse_steps, 0, 128);
	ImGui::SliderInt("SSR Num Precision Steps", (int*)&render_graph->getSSRData().cpu_data->num_precision_steps, 0, 128);
	ImGui::SliderFloat("SSR Facing Threshold", (float*)&render_graph->getSSRData().cpu_data->facing_threshold, 0.0f, 1.0f);
	ImGui::SliderFloat("SSR Bypass Depth Threshold", (float*)&render_graph->getSSRData().cpu_data->bypass_depth_threshold, 0.0f, 5.0f);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();

	ImGui::Begin("GBuffer");

	ImTextureID base_color_id = render_graph->fetchTextureID(render_graph->getGBuffer().base_color);
	ImTextureID normal_id = render_graph->fetchTextureID(render_graph->getGBuffer().normal);
	ImTextureID depth_id = render_graph->fetchTextureID(render_graph->getGBuffer().depth);
	ImTextureID shading_id = render_graph->fetchTextureID(render_graph->getGBuffer().shading);

	ImGui::BeginGroup();
		ImGui::Image(base_color_id, ImVec2(256, 256));
		ImGui::SameLine();
		ImGui::Image(normal_id, ImVec2(256, 256));
		ImGui::Image(depth_id, ImVec2(256, 256));
		ImGui::SameLine();
		ImGui::Image(shading_id, ImVec2(256, 256));
	ImGui::EndGroup();

	ImGui::End();

	ImGui::Begin("LBuffer");

	ImTextureID diffuse_id = render_graph->fetchTextureID(render_graph->getLBuffer().diffuse);
	ImTextureID specular_id = render_graph->fetchTextureID(render_graph->getLBuffer().specular);
	ImTextureID ssr_id = render_graph->fetchTextureID(render_graph->getSSRTrace().texture);
	ImTextureID ssao_blurred_id = render_graph->fetchTextureID(render_graph->getSSAOBlurred().texture);

	ImGui::BeginGroup();
		ImGui::Image(diffuse_id, ImVec2(256, 256));
		ImGui::SameLine();
		ImGui::Image(specular_id, ImVec2(256, 256));
		ImGui::Image(ssr_id, ImVec2(256, 256));
		ImGui::SameLine();
		ImGui::Image(ssao_blurred_id, ImVec2(256, 256));
	ImGui::EndGroup();

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
	render_graph->render(sponza, frame);

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
	window = glfwCreateWindow(1920, 1080, "Vulkan", nullptr, nullptr);

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
	resources = new ApplicationResources(driver, compiler);
	resources->init();

	sponza = new Scene(driver);
	sponza->import("scenes/pbr_sponza/sponza.obj");

	sky_light = new SkyLight(driver, resources->getSkyLightVertexShader(), resources->getSkyLightFragmentShader());
	sky_light->setBakedBRDFTexture(resources->getBakedBRDFTexture());
	sky_light->setEnvironmentCubemap(resources->getHDREnvironmentCubemap(0));
	sky_light->setIrradianceCubemap(resources->getHDRIrradianceCubemap(0));

	sponza->addLight(sky_light);
}

void Application::shutdownRenderScene()
{
	delete sky_light;
	sky_light = nullptr;

	delete resources;
	resources = nullptr;

	delete sponza;
	sponza = nullptr;
}

/*
 */
void Application::initRenderers()
{
	render_graph = new RenderGraph(driver, compiler);
	render_graph->init(resources, swap_chain->getWidth(), swap_chain->getHeight());
}

void Application::shutdownRenderers()
{
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
	compiler = new shaders::spirv::Compiler();
}

void Application::shutdownDriver()
{
	delete driver;
	driver = nullptr;

	delete compiler;
	compiler = nullptr;
}

/*
 */
void Application::initSwapChain()
{
#if defined(SCAPES_PLATFORM_WIN32)
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
}
