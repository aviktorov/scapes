#pragma once

#include <scapes/visual/Fwd.h>

#include <scapes/foundation/math/Math.h>
#include <scapes/foundation/game/World.h>
#include <scapes/foundation/game/Entity.h>

struct GLFWwindow;
class ApplicationFileSystem;
class ApplicationResources;
class ImGuiRenderer;
class SceneImporter;
class SwapChain;
class RenderPassImGui;
class RenderPassPost;

/*
 */
struct ApplicationState
{
	enum
	{
		MAX_TEMPORAL_FRAMES = 16,
	};

	int current_temporal_frame {0};
	int current_environment {0};
	scapes::foundation::math::vec2 temporal_samples[MAX_TEMPORAL_FRAMES];
	bool first_frame {true};
};

struct CameraState
{
	float phi {220.0f};
	float theta {20.0f};
	float radius {5.0f};
	scapes::foundation::math::vec3 target;
};

struct InputState
{
	const float rotation_speed {0.1f};
	const float scroll_speed {5.0f};
	bool rotating {false};
	float last_mouse_x {0.0f};
	float last_mouse_y {0.0f};
};

/*
 */
class Application
{
public:
	void run();

private:
	void initWindow();
	void shutdownWindow();

	void initDriver();
	void shutdownDriver();

	void initSwapChain();
	void shutdownSwapChain();
	void recreateSwapChain();

	void initRenderScene();
	void shutdownRenderScene();

	void initRenderers();
	void shutdownRenderers();

	void initImGui();
	void shutdownImGui();

	void update();
	void render();
	void postRender();
	void mainloop();

	static void onFramebufferResize(GLFWwindow *window, int width, int height);
	static void onMousePosition(GLFWwindow* window, double mouseX, double mouseY);
	static void onMouseButton(GLFWwindow* window, int button, int action, int mods);
	static void onScroll(GLFWwindow* window, double deltaX, double deltaY);

private:
	GLFWwindow *window {nullptr};
	bool window_resized {false};
	uint32_t width {0};
	uint32_t height {0};

	SceneImporter *importer {nullptr};
	scapes::foundation::game::Entity sky_light;
	scapes::foundation::game::Entity camera;

	ApplicationResources *application_resources {nullptr};
	ApplicationFileSystem *file_system {nullptr};

	ApplicationState application_state;
	CameraState camera_state;
	InputState input_state;

	SwapChain *swap_chain {nullptr};
	RenderPassImGui *imgui_pass {nullptr};

	scapes::foundation::render::Device *device {nullptr};
	scapes::foundation::shaders::Compiler *compiler {nullptr};
	scapes::foundation::game::World *world {nullptr};
	scapes::foundation::resources::ResourceManager *resource_manager {nullptr};

	scapes::visual::RenderGraph *render_graph {nullptr};
	scapes::visual::API *visual_api {nullptr};
};
