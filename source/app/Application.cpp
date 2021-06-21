#if defined(SCAPES_PLATFORM_WIN32)
	#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include "Application.h"
#include "ApplicationResources.h"

#include <render/shaders/spirv/Compiler.h>
#include <render/backend/Driver.h>
#include <render/SwapChain.h>

#include "SkyLight.h"
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

	const float width = static_cast<float>(swap_chain->getWidth());
	const float height = static_cast<float>(swap_chain->getHeight());
	const float aspect =  width / height;
	const float zNear = 0.1f;
	const float zFar = 10000.0f;

	glm::vec3 cameraPos;
	cameraPos.x = static_cast<float>(glm::cos(camera_state.phi) * glm::cos(camera_state.theta) * camera_state.radius);
	cameraPos.y = static_cast<float>(glm::sin(camera_state.phi) * glm::cos(camera_state.theta) * camera_state.radius);
	cameraPos.z = static_cast<float>(glm::sin(camera_state.theta) * camera_state.radius);

	glm::vec4 cameraParams;
	cameraParams.x = zNear;
	cameraParams.y = zFar;
	cameraParams.z = 1.0f / zNear;
	cameraParams.w = 1.0f / zFar;

	camera_state.view = glm::lookAt(cameraPos, zero, up);
	camera_state.projection = glm::perspective(glm::radians(60.0f), aspect, zNear, zFar);

	// projection matrix is adjusted for OpenGL so we need to flip it for non-flipped backends :)
	if (!driver->isFlipped())
		camera_state.projection[1][1] *= -1;

	// TODO: move to render graph
	// patch projection matrix for temporal supersampling
	const glm::vec2 &temporalSample = application_state.temporalSamples[application_state.currentTemporalFrame];
	camera_state.projection[2][0] = temporalSample.x / width;
	camera_state.projection[2][1] = temporalSample.y / height;

	application_state.currentTemporalFrame = (application_state.currentTemporalFrame + 1) % ApplicationState::MAX_TEMPORAL_FRAMES;

	camera_state.iview = glm::inverse(camera_state.view);
	camera_state.iprojection = glm::inverse(camera_state.projection);
	camera_state.cameraPosWS = cameraPos;
	camera_state.cameraParams = cameraParams;
	application_state.currentTime = time;

	if (application_state.firstFrame)
	{
		camera_state.viewOld = camera_state.view;
		application_state.firstFrame = false;
	}

	ImGui::Begin("Material Parameters");

	if (ImGui::Button("Reload Shaders"))
	{
		resources->reloadShaders();
		sky_light->setEnvironmentCubemap(resources->getHDREnvironmentCubemap(application_state.currentEnvironment));
		sky_light->setIrradianceCubemap(resources->getHDRIrradianceCubemap(application_state.currentEnvironment));
	}

	int oldCurrentEnvironment = application_state.currentEnvironment;
	if (ImGui::BeginCombo("Choose Your Destiny", resources->getHDRTexturePath(application_state.currentEnvironment)))
	{
		for (int i = 0; i < resources->getNumHDRTextures(); i++)
		{
			bool selected = (i == application_state.currentEnvironment);
			if (ImGui::Selectable(resources->getHDRTexturePath(i), &selected))
			{
				application_state.currentEnvironment = i;
				sky_light->setEnvironmentCubemap(resources->getHDREnvironmentCubemap(application_state.currentEnvironment));
				sky_light->setIrradianceCubemap(resources->getHDRIrradianceCubemap(application_state.currentEnvironment));
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::SliderFloat("Lerp User Material", &application_state.lerpUserValues, 0.0f, 1.0f);
	ImGui::SliderFloat("Metalness", &application_state.userMetalness, 0.0f, 1.0f);
	ImGui::SliderFloat("Roughness", &application_state.userRoughness, 0.0f, 1.0f);

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

	memcpy(camera_gpu_data, &camera_state, sizeof(CameraState));
	memcpy(frame.uniform_buffer_data, &application_state, sizeof(ApplicationState));

	render_graph->render(sponza, frame, camera_bindings);

	driver->submitSyncked(frame.command_buffer, swap_chain->getBackend());

	if (!swap_chain->present(frame) || windowResized)
	{
		windowResized = false;
		recreateSwapChain();
	}
}

void Application::postRender()
{
	camera_state.viewOld = camera_state.view;

	// TODO: call render_scene->postRender();
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
		postRender();
		glfwPollEvents();
	}

	driver->wait();
}

/*
 */
void Application::initWindow()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(800, 600, "Scapes v1.0", nullptr, nullptr);

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

	if (application->input_state.rotating)
	{
		double deltaX = mouseX - application->input_state.lastMouseX;
		double deltaY = mouseY - application->input_state.lastMouseY;

		application->camera_state.phi -= deltaX * application->input_state.rotationSpeed;
		application->camera_state.theta += deltaY * application->input_state.rotationSpeed;

		application->camera_state.phi = std::fmod(application->camera_state.phi, glm::two_pi<double>());
		application->camera_state.theta = std::clamp<double>(application->camera_state.theta, -glm::half_pi<double>(), glm::half_pi<double>());
	}

	application->input_state.lastMouseX = mouseX;
	application->input_state.lastMouseY = mouseY;
}

void Application::onMouseButton(GLFWwindow* window, int button, int action, int mods)
{
	Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
	assert(application != nullptr);

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
		application->input_state.rotating = (action == GLFW_PRESS);
}

void Application::onScroll(GLFWwindow* window, double deltaX, double deltaY)
{
	Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
	assert(application);

	application->camera_state.radius -= deltaY * application->input_state.scrollSpeed;
}

/*
 */
void Application::initRenderScene()
{
	resources = new ApplicationResources(driver, compiler);
	resources->init();

	sponza = new Scene(driver);
	sponza->import("scenes/pbr_sponza/sponza.obj");

	sky_light = new SkyLight(
		driver,
		resources->getShader(config::Shaders::FullscreenQuadVertex),
		resources->getShader(config::Shaders::SkylightDeferredFragment)
	);

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

	const uint8_t num_columns = ApplicationState::MAX_TEMPORAL_FRAMES / 4;
	const uint8_t num_rows = ApplicationState::MAX_TEMPORAL_FRAMES / num_columns;

	// Halton 2,3 sequence
	const float halton2[] = { 1.0f / 2.0f, 1.0f / 4.0f, 3.0f / 4.0f, 1.0f / 8.0f };
	const float halton3[] = { 1.0f / 3.0f, 2.0f / 3.0f, 1.0f / 9.0f, 4.0f / 9.0f };

	for (uint8_t y = 0; y < num_rows; ++y)
	{
		for (uint8_t x = 0; x < num_columns; ++x)
		{
			glm::vec2 &sample = application_state.temporalSamples[x + y * num_columns];
			sample.x = halton2[x];
			sample.y = halton3[y];

			sample = sample * 2.0f - 1.0f;
		}
	}

	camera_buffer = driver->createUniformBuffer(backend::BufferType::DYNAMIC, sizeof(CameraState));
	camera_bindings = driver->createBindSet();

	driver->bindUniformBuffer(camera_bindings, 0, camera_buffer);

	camera_gpu_data = driver->map(camera_buffer);
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
	driver = backend::Driver::create("PBR Sandbox", "Scape", render::backend::Api::VULKAN);
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
	application_state.firstFrame = true;
}
