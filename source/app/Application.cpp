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

/* TODO: remove later
 */
foundation::render::BottomLevelAccelerationStructure rt_blas = SCAPES_NULL_HANDLE;
foundation::render::TopLevelAccelerationStructure rt_tlas = SCAPES_NULL_HANDLE;
foundation::render::BindSet rt_bindings = SCAPES_NULL_HANDLE;
foundation::render::RayTracePipeline rt_pipeline = SCAPES_NULL_HANDLE;
foundation::render::StorageImage rt_image = SCAPES_NULL_HANDLE;
uint32_t rt_size = 512;

static void initRaytracing(foundation::render::Device *device, visual::API *visual_api, visual::RenderGraph *render_graph)
{
	visual::ShaderHandle rgen = visual_api->loadShader("shaders/test/test.rgen", foundation::render::ShaderType::RAY_GENERATION);
	visual::ShaderHandle miss = visual_api->loadShader("shaders/test/test.rmiss", foundation::render::ShaderType::MISS);
	visual::ShaderHandle closest_hit = visual_api->loadShader("shaders/test/test.rchit", foundation::render::ShaderType::CLOSEST_HIT);

	foundation::math::vec3 vertices[] =
	{
		{ 1.0f,  1.0f, 0.0f },
		{-1.0f,  1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f }
	};

	foundation::math::mat4 transform = foundation::math::mat4(1.0f);

	uint32_t indices[] = { 0, 1, 2 };

	foundation::render::AccelerationStructureGeometry geometry = {};
	geometry.num_vertices = 3;
	geometry.vertex_format = foundation::render::Format::R32G32B32_SFLOAT;
	geometry.vertices = vertices;
	geometry.index_format = foundation::render::IndexFormat::UINT32;
	geometry.num_indices = 3;
	geometry.indices = indices;
	memcpy(geometry.transform, &transform, sizeof(float) * 16);

	rt_blas = device->createBottomLevelAccelerationStructure(1, &geometry);

	foundation::render::AccelerationStructureInstance instance = {};
	instance.blas = rt_blas;
	memcpy(instance.transform, &transform, sizeof(float) * 16);

	rt_tlas = device->createTopLevelAccelerationStructure(1, &instance);

	rt_image = device->createStorageImage(rt_size, rt_size, foundation::render::Format::R8G8B8A8_UNORM);

	rt_bindings = device->createBindSet();
	device->bindTopLevelAccelerationStructure(rt_bindings, 0, rt_tlas);
	device->bindStorageImage(rt_bindings, 1, rt_image);

	rt_pipeline = device->createRayTracePipeline();
	device->setBindSet(rt_pipeline, 0, rt_bindings);
	device->setBindSet(rt_pipeline, 1, render_graph->getGroupBindings("Camera"));
	device->addRaygenShader(rt_pipeline, rgen->shader);
	device->addMissShader(rt_pipeline, miss->shader);
	device->addHitGroupShader(rt_pipeline, SCAPES_NULL_HANDLE, SCAPES_NULL_HANDLE, closest_hit->shader);

	device->flush(rt_pipeline);
}

static void shutdownRaytracing(foundation::render::Device *device)
{
	device->destroyTopLevelAccelerationStructure(rt_tlas);
	rt_tlas = SCAPES_NULL_HANDLE;

	device->destroyBottomLevelAccelerationStructure(rt_blas);
	rt_blas = SCAPES_NULL_HANDLE;

	device->destroyStorageImage(rt_image);
	rt_image = SCAPES_NULL_HANDLE;

	device->destroyRayTracePipeline(rt_pipeline);
	rt_pipeline = SCAPES_NULL_HANDLE;

	device->destroyBindSet(rt_bindings);
	rt_bindings = SCAPES_NULL_HANDLE;
}

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
	initRaytracing(device, visual_api, render_graph);
	mainloop();
	shutdownRaytracing(device);
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

	float cos_phi = cos(foundation::math::radians(camera_state.phi));
	float sin_phi = sin(foundation::math::radians(camera_state.phi));
	float cos_theta = cos(foundation::math::radians(camera_state.theta));
	float sin_theta = sin(foundation::math::radians(camera_state.theta));

	foundation::math::vec3 camera_position;
	camera_position.x = sin_phi * cos_theta * camera_state.radius;
	camera_position.y = cos_phi * cos_theta * camera_state.radius;
	camera_position.z = sin_theta * camera_state.radius;

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
	const foundation::math::vec2 &temporal_sample = application_state.temporal_samples[application_state.current_temporal_frame];
	projection[2][0] = temporal_sample.x / width;
	projection[2][1] = temporal_sample.y / height;

	application_state.current_temporal_frame = (application_state.current_temporal_frame + 1) % ApplicationState::MAX_TEMPORAL_FRAMES;

	render_graph->setGroupParameter("Camera", "View", view);
	render_graph->setGroupParameter("Camera", "IView", foundation::math::inverse(view));
	render_graph->setGroupParameter("Camera", "Projection", projection);
	render_graph->setGroupParameter("Camera", "IProjection", foundation::math::inverse(projection));
	render_graph->setGroupParameter("Camera", "Parameters", camera_parameters);
	render_graph->setGroupParameter("Camera", "PositionWS", camera_position);

	render_graph->setGroupParameter("Application", "Time", time);

	if (application_state.first_frame)
	{
		render_graph->setGroupParameter<foundation::math::mat4>("Camera", "ViewOld", view);
		application_state.first_frame = false;
	}

	bool reset_environment = false;

	ImGui::Begin("Render Parameters");

	int oldcurrent_environment = application_state.current_environment;
	if (ImGui::BeginCombo("Choose Your Destiny", application_resources->getIBLTexturePath(application_state.current_environment)))
	{
		for (int i = 0; i < application_resources->getNumIBLTextures(); i++)
		{
			bool selected = (i == application_state.current_environment);
			if (ImGui::Selectable(application_resources->getIBLTexturePath(i), &selected))
			{
				application_state.current_environment = i;
				reset_environment = true;
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	//
	float override_base_color = render_graph->getGroupParameter<float>("Application", "OverrideBaseColor");
	float override_shading = render_graph->getGroupParameter<float>("Application", "OverrideShading");
	float user_metalness = render_graph->getGroupParameter<float>("Application", "UserMetalness");
	float user_roughness = render_graph->getGroupParameter<float>("Application", "UserRoughness");

	ImGui::SliderFloat("Override Base Color", &override_base_color, 0.0f, 1.0f);
	ImGui::SliderFloat("Override Shading", &override_shading, 0.0f, 1.0f);
	ImGui::SliderFloat("User Metalness", &user_metalness, 0.0f, 1.0f);
	ImGui::SliderFloat("User Roughness", &user_roughness, 0.0f, 1.0f);

	render_graph->setGroupParameter("Application", "OverrideBaseColor", override_base_color);
	render_graph->setGroupParameter("Application", "OverrideShading", override_shading);
	render_graph->setGroupParameter("Application", "UserMetalness", user_metalness);
	render_graph->setGroupParameter("Application", "UserRoughness", user_roughness);

	//
	float ssao_radius = render_graph->getGroupParameter<float>("SSAO", "Radius");
	float ssao_intensity = render_graph->getGroupParameter<float>("SSAO", "Intensity");
	int ssao_num_samples = render_graph->getGroupParameter<int>("SSAO", "NumSamples");

	ImGui::SliderFloat("SSAO Radius", &ssao_radius, 0.0f, 100.0f);
	ImGui::SliderFloat("SSAO Intensity", &ssao_intensity, 0.0f, 100.0f);
	ImGui::SliderInt("SSAO Samples", &ssao_num_samples, 1, 32);

	render_graph->setGroupParameter("SSAO", "Radius", ssao_radius);
	render_graph->setGroupParameter("SSAO", "Intensity", ssao_intensity);
	render_graph->setGroupParameter("SSAO", "NumSamples", ssao_num_samples);

	//
	float ssr_coarse_step = render_graph->getGroupParameter<float>("SSR", "CoarseStep");
	int ssr_num_coarse_steps = render_graph->getGroupParameter<int>("SSR", "NumCoarseSteps");
	int ssr_num_precision_steps = render_graph->getGroupParameter<int>("SSR", "NumPrecisionSteps");
	float ssr_facing_threshold = render_graph->getGroupParameter<float>("SSR", "FacingThreshold");
	float ssr_depth_bypass_threshold = render_graph->getGroupParameter<float>("SSR", "DepthBypassThreshold");
	float ssr_brdf_bias = render_graph->getGroupParameter<float>("SSR", "BRDFBias");
	float ssr_min_step_multiplier = render_graph->getGroupParameter<float>("SSR", "MinStepMultiplier");
	float ssr_max_step_multiplier = render_graph->getGroupParameter<float>("SSR", "MaxStepMultiplier");

	ImGui::SliderFloat("SSR Coarse Step", &ssr_coarse_step, 0.0f, 100.0f);
	ImGui::SliderInt("SSR Num Coarse Steps", &ssr_num_coarse_steps, 0, 128);
	ImGui::SliderInt("SSR Num Precision Steps", &ssr_num_precision_steps, 0, 16);
	ImGui::SliderFloat("SSR Facing Threshold", &ssr_facing_threshold, 0.0f, 1.0f);
	ImGui::SliderFloat("SSR Depth Bypass Threshold", &ssr_depth_bypass_threshold, 0.0f, 5.0f);
	ImGui::SliderFloat("SSR BRDF Bias", &ssr_brdf_bias, 0.0f, 1.0f);
	ImGui::SliderFloat("SSR Min Step Multiplier", &ssr_min_step_multiplier, 0.0f, 10.0f);
	ImGui::SliderFloat("SSR Max Step Multiplier", &ssr_max_step_multiplier, 0.0f, 10.0f);

	render_graph->setGroupParameter("SSR", "CoarseStep", ssr_coarse_step);
	render_graph->setGroupParameter("SSR", "NumCoarseSteps", ssr_num_coarse_steps);
	render_graph->setGroupParameter("SSR", "NumPrecisionSteps", ssr_num_precision_steps);
	render_graph->setGroupParameter("SSR", "FacingThreshold", ssr_facing_threshold);
	render_graph->setGroupParameter("SSR", "DepthBypassThreshold", ssr_depth_bypass_threshold);
	render_graph->setGroupParameter("SSR", "BRDFBias", ssr_brdf_bias);
	render_graph->setGroupParameter("SSR", "MinStepMultiplier", ssr_min_step_multiplier);
	render_graph->setGroupParameter("SSR", "MaxStepMultiplier", ssr_max_step_multiplier);

	//
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();

	ImGui::Begin("RT");

	ImTextureID rt_image_id = imgui_pass->fetchStorageImageID(rt_image);

	ImGui::Image(rt_image_id, ImVec2(static_cast<float>(rt_size), static_cast<float>(rt_size)));

	ImGui::End();

	ImGui::Begin("GBuffer");

	ImTextureID base_color_id = imgui_pass->fetchTextureID(render_graph->getRenderBufferTexture("GBufferBaseColor"));
	ImTextureID normal_id = imgui_pass->fetchTextureID(render_graph->getRenderBufferTexture("GBufferNormal"));
	ImTextureID depth_id = imgui_pass->fetchTextureID(render_graph->getRenderBufferTexture("GBufferDepth"));
	ImTextureID shading_id = imgui_pass->fetchTextureID(render_graph->getRenderBufferTexture("GBufferShading"));

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

	ImTextureID diffuse_id = imgui_pass->fetchTextureID(render_graph->getRenderBufferTexture("LBufferDiffuse"));
	ImTextureID specular_id = imgui_pass->fetchTextureID(render_graph->getRenderBufferTexture("LBufferSpecular"));
	ImTextureID ssr_id = imgui_pass->fetchTextureID(render_graph->getRenderBufferTexture("SSRTAA"));
	ImTextureID ssao_id = imgui_pass->fetchTextureID(render_graph->getRenderBufferTexture("SSAO"));

	ImGui::BeginGroup();
		ImGui::Image(diffuse_id, ImVec2(256, 256));
		ImGui::SameLine();
		ImGui::Image(specular_id, ImVec2(256, 256));
		ImGui::Image(ssr_id, ImVec2(256, 256));
		ImGui::SameLine();
		ImGui::Image(ssao_id, ImVec2(256, 256));
	ImGui::EndGroup();

	ImGui::End();

	if (reset_environment)
	{
		visual::components::SkyLight &comp = sky_light.getComponent<visual::components::SkyLight>();
		comp.ibl_environment = application_resources->getIBLTexture(application_state.current_environment);
	}
}

/*
 */
void Application::render()
{
	foundation::render::CommandBuffer command_buffer = swap_chain->acquire();

	if (!command_buffer)
		return;

	device->resetCommandBuffer(command_buffer);
	device->beginCommandBuffer(command_buffer);

	render_graph->render(command_buffer);
	device->traceRays(command_buffer, rt_pipeline, rt_size, rt_size, 1);

	device->endCommandBuffer(command_buffer);
	device->submitSyncked(command_buffer, swap_chain->getBackend());

	if (!swap_chain->present(command_buffer) || window_resized)
	{
		window_resized = false;
		recreateSwapChain();
	}
}

void Application::postRender()
{
	const foundation::math::mat4 &view = render_graph->getGroupParameter<foundation::math::mat4>("Camera", "View");
	render_graph->setGroupParameter<foundation::math::mat4>("Camera", "ViewOld", view);
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

	device->wait();
}

/*
 */
void Application::initWindow()
{
	width = 1920;
	height = 1080;

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

	application->window_resized = true;
	application->width = static_cast<uint32_t>(width);
	application->height = static_cast<uint32_t>(height);
}

void Application::onMousePosition(GLFWwindow* window, double mouse_x, double mouse_y)
{
	Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
	assert(application != nullptr);

	if (application->input_state.rotating)
	{
		float delta_x = static_cast<float>(mouse_x) - application->input_state.last_mouse_x;
		float delta_y = static_cast<float>(mouse_y) - application->input_state.last_mouse_y;

		application->camera_state.phi += delta_x * application->input_state.rotation_speed;
		application->camera_state.theta += delta_y * application->input_state.rotation_speed;

		application->camera_state.phi = std::fmod(application->camera_state.phi, 360.0f);
		application->camera_state.theta = std::clamp(application->camera_state.theta, -90.0f, 90.0f);
	}

	application->input_state.last_mouse_x = static_cast<float>(mouse_x);
	application->input_state.last_mouse_y = static_cast<float>(mouse_y);
}

void Application::onMouseButton(GLFWwindow *window, int button, int action, int mods)
{
	Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
	assert(application != nullptr);

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
		application->input_state.rotating = (action == GLFW_PRESS);
}

void Application::onScroll(GLFWwindow *window, double delta_x, double delta_y)
{
	Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
	assert(application);
 
	application->camera_state.radius -= static_cast<float>(delta_y) * application->input_state.scroll_speed;
}

/*
 */
void Application::initRenderScene()
{
	resource_manager = foundation::resources::ResourceManager::create(file_system);

	world = foundation::game::World::create();
	visual_api = visual::API::create(resource_manager, world, device, compiler);

	application_resources = new ApplicationResources(visual_api);
	application_resources->init();

	importer = new SceneImporter(world, visual_api);
	importer->importCGLTF("assets/scenes/sphere.glb", application_resources);

	sky_light = foundation::game::Entity(world);
	sky_light.addComponent<visual::components::SkyLight>(
		application_resources->getIBLTexture(0)
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
	render_graph = visual::RenderGraph::create(visual_api, file_system);
	render_graph->registerRenderPassType<RenderPassPrepareOld>();
	render_graph->registerRenderPassType<RenderPassGeometry>();
	render_graph->registerRenderPassType<RenderPassLBuffer>();
	render_graph->registerRenderPassType<RenderPassPost>();
	render_graph->registerRenderPassType<RenderPassImGui>();
	render_graph->registerRenderPassType<RenderPassSwapRenderBuffers>();

	render_graph->load("shaders/render_graph/schema.yaml");

	// ImGui pass
	imgui_pass = render_graph->getRenderPass<RenderPassImGui>("ImGui");
	imgui_pass->setImGuiContext(ImGui::GetCurrentContext());

	// setup ssao
	constexpr uint32_t MAX_SSAO_SAMPLES = 32;

	uint32_t data[16];
	for (uint32_t i = 0; i < 16; ++i)
	{
		const foundation::math::vec2 &noise = foundation::math::circularRand(1.0f);
		data[i] = foundation::math::packHalf2x16(noise);
	}

	visual::TextureHandle ssao_noise = visual_api->createTexture2D(foundation::render::Format::R16G16_SFLOAT, 4, 4, 1, data);

	foundation::math::vec4 samples[MAX_SSAO_SAMPLES];
	float inum_samples = 1.0f / static_cast<float>(MAX_SSAO_SAMPLES);

	for (uint32_t i = 0; i < MAX_SSAO_SAMPLES; ++i)
	{
		float scale = i * inum_samples;
		scale = foundation::math::mix(0.1f, 1.0f, scale * scale);

		float radius = foundation::math::linearRand(0.0f, 1.0f) * scale;

		float phi = foundation::math::radians(foundation::math::linearRand(0.0f, 360.0f));
		float theta = foundation::math::radians(foundation::math::linearRand(0.0f, 90.0f));

		float cos_theta = cos(theta);
		samples[i].x = sin(phi) * cos_theta * radius;
		samples[i].y = cos(phi) * cos_theta * radius;
		samples[i].z = sin(theta) * radius;
		samples[i].w = 1.0f;
	}

	render_graph->setGroupTexture("SSAO", "Noise", ssao_noise);
	render_graph->setGroupParameter<foundation::math::vec4>("SSAO", "Samples", MAX_SSAO_SAMPLES, samples);

	// init render graph
	render_graph->setSwapChain(swap_chain->getBackend());
	render_graph->init(width, height);

	// setup temporal frames
	const uint8_t num_columns = ApplicationState::MAX_TEMPORAL_FRAMES / 4;
	const uint8_t num_rows = ApplicationState::MAX_TEMPORAL_FRAMES / num_columns;

	const float halton2[4] = { 1.0f / 2.0f, 1.0f / 4.0f, 3.0f / 4.0f, 1.0f / 8.0f };
	const float halton3[4] = { 1.0f / 3.0f, 2.0f / 3.0f, 1.0f / 9.0f, 4.0f / 9.0f };

	for (uint8_t y = 0; y < num_rows; ++y)
	{
		for (uint8_t x = 0; x < num_columns; ++x)
		{
			foundation::math::vec2 &sample = application_state.temporal_samples[x + y * num_columns];
			sample.x = halton2[x];
			sample.y = halton3[y];

			sample = sample * 2.0f - 1.0f;
		}
	}
}

void Application::shutdownRenderers()
{
	imgui_pass = nullptr;

	visual::RenderGraph::destroy(render_graph);
	render_graph = nullptr;
}

/*
 */
void Application::initImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOther(window, true);
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
	render_graph->setSwapChain(swap_chain->getBackend());
	imgui_pass->invalidateTextureIDs();
	imgui_pass->invalidateStorageImageIDs();
	render_graph->resize(width, height);
	application_state.first_frame = true;
}
