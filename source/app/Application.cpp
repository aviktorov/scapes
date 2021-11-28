#include "Application.h"
#include "ApplicationResources.h"
#include "IO.h"

#include <scapes/foundation/shaders/Compiler.h>
#include <scapes/foundation/render/Device.h>

#include <scapes/visual/API.h>
#include <scapes/visual/Components.h>
#include <scapes/visual/RenderGraph.h>

#include "SwapChain.h"
#include "RenderPasses.h"
#include "SceneImporter.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"

#include <algorithm>
#include <iostream>
#include <chrono>

using namespace scapes;

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
	static auto start_time = std::chrono::high_resolution_clock::now();
	auto current_time = std::chrono::high_resolution_clock::now();

	float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

	const foundation::math::vec3 &up = {0.0f, 0.0f, 1.0f};
	const foundation::math::vec3 &zero = {0.0f, 0.0f, 0.0f};

	const float viewport_width = static_cast<float>(width);
	const float viewport_height = static_cast<float>(height);
	const float aspect =  viewport_width / viewport_height;
	const float zNear = 0.1f;
	const float zFar = 10000.0f;
	const float fov = 60.0f;

	foundation::math::vec3 camera_position;
	camera_position.x = static_cast<float>(foundation::math::cos(camera_state.phi) * foundation::math::cos(camera_state.theta) * camera_state.radius);
	camera_position.y = static_cast<float>(foundation::math::sin(camera_state.phi) * foundation::math::cos(camera_state.theta) * camera_state.radius);
	camera_position.z = static_cast<float>(foundation::math::sin(camera_state.theta) * camera_state.radius);

	foundation::math::vec4 camera_parameters;
	camera_parameters.x = zNear;
	camera_parameters.y = zFar;
	camera_parameters.z = 1.0f / zNear;
	camera_parameters.w = 1.0f / zFar;

	foundation::math::mat4 view = foundation::math::lookAt(camera_position, zero, up);
	foundation::math::mat4 projection = foundation::math::perspective(foundation::math::radians(fov), aspect, zNear, zFar);

	// projection matrix is adjusted for OpenGL so we need to flip it for non-flipped backends :)
	if (!device->isFlipped())
		projection[1][1] *= -1;

	// TODO: move to render graph
	// patch projection matrix for temporal supersampling
	const foundation::math::vec2 &temporalSample = application_state.temporalSamples[application_state.currentTemporalFrame];
	projection[2][0] = temporalSample.x / width;
	projection[2][1] = temporalSample.y / height;

	application_state.currentTemporalFrame = (application_state.currentTemporalFrame + 1) % ApplicationState::MAX_TEMPORAL_FRAMES;

	new_render_graph->setParameterValue("Camera", "View", view);
	new_render_graph->setParameterValue("Camera", "IView", foundation::math::inverse(view));
	new_render_graph->setParameterValue("Camera", "Projection", projection);
	new_render_graph->setParameterValue("Camera", "IProjection", foundation::math::inverse(projection));
	new_render_graph->setParameterValue("Camera", "Parameters", camera_parameters);
	new_render_graph->setParameterValue("Camera", "PositionWS", camera_position);

	new_render_graph->setParameterValue("Application", "CurrentTime", time);

	if (application_state.firstFrame)
	{
		new_render_graph->setParameterValue<foundation::math::mat4>("Camera", "ViewOld", view);
		application_state.firstFrame = false;
	}

	bool reset_environment = false;
	/*
	ImGui::Begin("Material Parameters");

	int oldCurrentEnvironment = application_state.currentEnvironment;
	if (ImGui::BeginCombo("Choose Your Destiny", application_resources->getIBLTexturePath(application_state.currentEnvironment)))
	{
		for (int i = 0; i < application_resources->getNumIBLTextures(); i++)
		{
			bool selected = (i == application_state.currentEnvironment);
			if (ImGui::Selectable(application_resources->getIBLTexturePath(i), &selected))
			{
				application_state.currentEnvironment = i;
				reset_environment = true;
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	/**/

	float override_base_color = new_render_graph->getParameterValue<float>("Application", "OverrideBaseColor");
	float override_shading = new_render_graph->getParameterValue<float>("Application", "OverrideShading");
	float user_metalness = new_render_graph->getParameterValue<float>("Application", "UserMetalness");
	float user_roughness = new_render_graph->getParameterValue<float>("Application", "UserRoughness");

	override_base_color = 0.0f;
	override_shading = 0.0f;
	user_metalness = 0.0f;
	user_roughness = 0.0f;

	/*
	ImGui::SliderFloat("Override Base Color", &override_base_color, 0.0f, 1.0f);
	ImGui::SliderFloat("Override Shading", &override_shading, 0.0f, 1.0f);
	ImGui::SliderFloat("User Metalness", &user_metalness, 0.0f, 1.0f);
	ImGui::SliderFloat("User Roughness", &user_roughness, 0.0f, 1.0f);
	/**/

	new_render_graph->setParameterValue("Application", "OverrideBaseColor", override_base_color);
	new_render_graph->setParameterValue("Application", "OverrideShading", override_shading);
	new_render_graph->setParameterValue("Application", "UserMetalness", user_metalness);
	new_render_graph->setParameterValue("Application", "UserRoughness", user_roughness);

	/*
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
	/**/

	/*
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
	/**/

	if (reset_environment)
	{
		visual::components::SkyLight &comp = sky_light.getComponent<visual::components::SkyLight>();
		comp.ibl_environment = application_resources->getIBLTexture(application_state.currentEnvironment);
	}
}

/*
 */
void Application::render()
{
	foundation::render::CommandBuffer *command_buffer = swap_chain->acquire();

	if (!command_buffer)
	{
		recreateSwapChain();
		return;
	}

	device->resetCommandBuffer(command_buffer);
	device->beginCommandBuffer(command_buffer);

	new_render_graph->render(command_buffer);

	device->endCommandBuffer(command_buffer);

	device->submitSyncked(command_buffer, swap_chain->getBackend());

	if (!swap_chain->present(command_buffer) || windowResized)
	{
		windowResized = false;
		recreateSwapChain();
	}
}

void Application::postRender()
{
	const foundation::math::mat4 &view = new_render_graph->getParameterValue<foundation::math::mat4>("Camera", "View");
	new_render_graph->setParameterValue<foundation::math::mat4>("Camera", "ViewOld", view);
}

/*
 */
void Application::mainloop()
{
	if (!window)
		return;

	while (!glfwWindowShouldClose(window))
	{
		// ImGui_ImplGlfw_NewFrame();
		// ImGui::NewFrame();

		update();

		// ImGui::Render();

		render();
		postRender();
		glfwPollEvents();
	}

	device->wait();
}

/*
 */
void Application::initWindow()
{
	width = 800;
	height = 600;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, "Scapes v1.0", nullptr, nullptr);

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
	if (width == 0 || height == 0)
		return;

	Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
	assert(application != nullptr);

	application->windowResized = true;
	application->width = static_cast<uint32_t>(width);
	application->height = static_cast<uint32_t>(height);
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

		application->camera_state.phi = std::fmod(application->camera_state.phi, foundation::math::two_pi<double>());
		application->camera_state.theta = std::clamp<double>(application->camera_state.theta, -foundation::math::half_pi<double>(), foundation::math::half_pi<double>());
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
	resource_manager = foundation::resources::ResourceManager::create();

	world = foundation::game::World::create();
	visual_api = visual::API::create(resource_manager, world, device, compiler);

	application_resources = new ApplicationResources(device, visual_api);
	application_resources->init();

	importer = new SceneImporter(world, visual_api);
	importer->importCGLTF("assets/scenes/blender_splash/blender_splash.glb", application_resources);

	sky_light = foundation::game::Entity(world);
	sky_light.addComponent<visual::components::SkyLight>(
		application_resources->getIBLTexture(0),
		application_resources->getFullscreenQuad(),
		application_resources->getShader(config::Shaders::FullscreenQuadVertex),
		application_resources->getShader(config::Shaders::SkylightDeferredFragment)
	);
}

void Application::shutdownRenderScene()
{
	delete application_resources;
	application_resources = nullptr;

	delete importer;
	importer = nullptr;

	visual::API::destroy(visual_api);
	visual_api = nullptr;

	foundation::game::World::destroy(world);
	world = nullptr;

	foundation::resources::ResourceManager::destroy(resource_manager);
	resource_manager = nullptr;
}

/*
 */
void Application::initRenderers()
{
	new_render_graph = visual::RenderGraph::create(device, world);

	new_render_graph->addParameterGroup("Application");
	new_render_graph->addParameter("Application", "OverrideBaseColor", sizeof(float));
	new_render_graph->addParameter("Application", "OverrideShading", sizeof(float));
	new_render_graph->addParameter("Application", "UserMetalness", sizeof(float));
	new_render_graph->addParameter("Application", "UserRoughness", sizeof(float));
	new_render_graph->addParameter("Application", "CurrentTime", sizeof(float));

	new_render_graph->addParameterGroup("Camera");
	new_render_graph->addParameter("Camera", "View", sizeof(foundation::math::mat4));
	new_render_graph->addParameter("Camera", "IView", sizeof(foundation::math::mat4));
	new_render_graph->addParameter("Camera", "Projection", sizeof(foundation::math::mat4));
	new_render_graph->addParameter("Camera", "IProjection", sizeof(foundation::math::mat4));
	new_render_graph->addParameter("Camera", "ViewOld", sizeof(foundation::math::mat4));
	new_render_graph->addParameter("Camera", "Parameters", sizeof(foundation::math::vec4));
	new_render_graph->addParameter("Camera", "PositionWS", sizeof(foundation::math::vec3));

	constexpr uint32_t MAX_SSAO_SAMPLES = 64;

	new_render_graph->addParameterGroup("SSAOKernel");
	new_render_graph->addParameter("SSAOKernel", "NumSamples", sizeof(uint32_t));
	new_render_graph->addParameter("SSAOKernel", "Radius", sizeof(float));
	new_render_graph->addParameter("SSAOKernel", "Intensity", sizeof(float));
	new_render_graph->addParameter("SSAOKernel", "Samples", sizeof(foundation::math::vec4) * MAX_SSAO_SAMPLES);

	new_render_graph->addTextureRenderBuffer("GBufferBaseColor", foundation::render::Format::R8G8B8A8_UNORM, 1);
	new_render_graph->addTextureRenderBuffer("GBufferShading", foundation::render::Format::R8G8_UNORM, 1);
	new_render_graph->addTextureRenderBuffer("GBufferNormal", foundation::render::Format::R16G16B16A16_SFLOAT, 1);
	new_render_graph->addTextureRenderBuffer("GBufferDepth", foundation::render::Format::D32_SFLOAT, 1);
	new_render_graph->addTextureRenderBuffer("GBufferVelocity", foundation::render::Format::R16G16_SFLOAT, 1);

	new_render_graph->addTextureRenderBuffer("SSAONoised", foundation::render::Format::R8G8B8A8_UNORM, 1);
	new_render_graph->addTextureRenderBuffer("SSAO", foundation::render::Format::R8G8B8A8_UNORM, 1);

	new_render_graph->addTextureResource("SSAOKernelNoise", visual::TextureHandle());

	RenderPassGeometry *gbuffer_pass = new RenderPassGeometry();
	auto gbuffer_load_op = foundation::render::RenderPassLoadOp::CLEAR;
	auto gbuffer_store_op = foundation::render::RenderPassStoreOp::STORE;

	gbuffer_pass->setDepthStencilOutput("GBufferDepth", gbuffer_load_op, gbuffer_store_op, {1.0f, 0});
	gbuffer_pass->addColorOutput("GBufferBaseColor", gbuffer_load_op, gbuffer_store_op, {0.0f, 0.0f, 0.0f, 0.0f});
	gbuffer_pass->addColorOutput("GBufferNormal", gbuffer_load_op, gbuffer_store_op, {0.0f, 0.0f, 0.0f, 0.0f});
	gbuffer_pass->addColorOutput("GBufferShading", gbuffer_load_op, gbuffer_store_op, {0.0f, 0.0f, 0.0f, 0.0f});
	gbuffer_pass->addColorOutput("GBufferVelocity", gbuffer_load_op, gbuffer_store_op, {0.0f, 0.0f, 0.0f, 0.0f});

	gbuffer_pass->addInputParameterGroup("Application");
	gbuffer_pass->addInputParameterGroup("Camera");

	gbuffer_pass->setMaterialBinding(2);
	gbuffer_pass->setVertexShader(application_resources->getShader(config::Shaders::GBufferVertex));
	gbuffer_pass->setFragmentShader(application_resources->getShader(config::Shaders::GBufferFragment));

	new_render_graph->addRenderPass(gbuffer_pass);

	RenderPassPost *ssao_pass = new RenderPassPost();

	ssao_pass->setFragmentShader(application_resources->getShader(config::Shaders::SSAOFragment));
	ssao_pass->setFullscreenQuad(application_resources->getShader(config::Shaders::FullscreenQuadVertex), application_resources->getFullscreenQuad());

	ssao_pass->addColorOutput("SSAONoised", foundation::render::RenderPassLoadOp::DONT_CARE, foundation::render::RenderPassStoreOp::STORE, {});

	ssao_pass->addInputParameterGroup("Camera");
	ssao_pass->addInputParameterGroup("SSAOKernel");
	ssao_pass->addInputTexture("GBufferDepth");
	ssao_pass->addInputTexture("GBufferNormal");
	ssao_pass->addInputTexture("SSAOKernelNoise");

	new_render_graph->addRenderPass(ssao_pass);

	RenderPassPost *ssao_blur_pass = new RenderPassPost();

	ssao_blur_pass->setFragmentShader(application_resources->getShader(config::Shaders::SSAOBlurFragment));
	ssao_blur_pass->setFullscreenQuad(application_resources->getShader(config::Shaders::FullscreenQuadVertex), application_resources->getFullscreenQuad());

	ssao_blur_pass->addColorOutput("SSAO", foundation::render::RenderPassLoadOp::DONT_CARE, foundation::render::RenderPassStoreOp::STORE, {});

	ssao_blur_pass->addInputTexture("SSAONoised");

	new_render_graph->addRenderPass(ssao_blur_pass);

	// setup ssao kernel
	uint32_t data[16];
	for (uint32_t i = 0; i < 16; ++i)
	{
		const foundation::math::vec2 &noise = foundation::math::circularRand(1.0f);
		data[i] = foundation::math::packHalf2x16(noise);
	}

	visual::TextureHandle ssao_noise = visual_api->createTexture2D(foundation::render::Format::R16G16_SFLOAT, 4, 4, 1, data);

	constexpr uint32_t SSAO_SAMPLES = 32;

	new_render_graph->setTextureResource("SSAOKernelNoise", ssao_noise);
	new_render_graph->setParameterValue<uint32_t>("SSAOKernel", "NumSamples", SSAO_SAMPLES);
	new_render_graph->setParameterValue<float>("SSAOKernel", "Intensity", 1.5f);
	new_render_graph->setParameterValue<float>("SSAOKernel", "Radius", 100.0f);

	foundation::math::vec4 samples[SSAO_SAMPLES];
	float inum_samples = 1.0f / static_cast<float>(SSAO_SAMPLES);

	for (uint32_t i = 0; i < SSAO_SAMPLES; ++i)
	{
		float scale = i * inum_samples;
		scale = foundation::math::mix(0.1f, 1.0f, scale * scale);

		float radius = foundation::math::linearRand(0.0f, 1.0f) * scale;

		float phi = foundation::math::radians(foundation::math::linearRand(0.0f, 360.0f));
		float theta = foundation::math::radians(foundation::math::linearRand(0.0f, 90.0f));

		float cos_theta = cos(theta);
		samples[i].x = cos(phi) * cos_theta * radius;
		samples[i].y = sin(phi) * cos_theta * radius;
		samples[i].z = sin(theta) * radius;
		samples[i].w = 1.0f;
	}

	new_render_graph->setParameterValue<foundation::math::vec4>("SSAOKernel", "Samples", SSAO_SAMPLES, samples);

	new_render_graph->init(width, height);

	// setup temporal frames
	const uint8_t num_columns = ApplicationState::MAX_TEMPORAL_FRAMES / 4;
	const uint8_t num_rows = ApplicationState::MAX_TEMPORAL_FRAMES / num_columns;

	const float halton2[4] = { 1.0f / 2.0f, 1.0f / 4.0f, 3.0f / 4.0f, 1.0f / 8.0f };
	const float halton3[4] = { 1.0f / 3.0f, 2.0f / 3.0f, 1.0f / 9.0f, 4.0f / 9.0f };

	for (uint8_t y = 0; y < num_rows; ++y)
	{
		for (uint8_t x = 0; x < num_columns; ++x)
		{
			foundation::math::vec2 &sample = application_state.temporalSamples[x + y * num_columns];
			sample.x = halton2[x];
			sample.y = halton3[y];

			sample = sample * 2.0f - 1.0f;
		}
	}
}

void Application::shutdownRenderers()
{
	// TODO: remove all render passes to avoid memory leak
	visual::RenderGraph::destroy(new_render_graph);
	new_render_graph = nullptr;
}

/*
 */
void Application::initImGui()
{
	IMGUI_CHECKVERSION();
	// ImGui::CreateContext();
	// ImGui::StyleColorsDark();

	// ImGui_ImplGlfw_InitForVulkan(window, true);
}

void Application::shutdownImGui()
{
	// ImGui_ImplGlfw_Shutdown();
	// ImGui::DestroyContext();
}

/*
 */
void Application::initDriver()
{
	file_system = new ApplicationFileSystem("assets/");

	device = foundation::render::Device::create("PBR Sandbox", "Scape", foundation::render::Api::VULKAN);
	compiler = foundation::shaders::Compiler::create(foundation::shaders::ShaderILType::SPIRV, file_system);
}

void Application::shutdownDriver()
{
	foundation::render::Device::destroy(device);
	device = nullptr;

	foundation::shaders::Compiler::destroy(compiler);
	compiler = nullptr;

	delete file_system;
	file_system = nullptr;
}

/*
 */
void Application::initSwapChain()
{
#if defined(GLFW_EXPOSE_NATIVE_WIN32)
	void *nativeWindow = glfwGetWin32Window(window);
#else
	void *nativeWindow = nullptr;
#endif

	if (!swap_chain)
		swap_chain = new SwapChain(device, nativeWindow);

	swap_chain->init();
}

void Application::shutdownSwapChain()
{
	delete swap_chain;
	swap_chain = nullptr;
}

void Application::recreateSwapChain()
{
	device->wait();

	swap_chain->recreate();
	new_render_graph->resize(width, height);
	application_state.firstFrame = true;
}
