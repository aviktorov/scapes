#pragma once

#include <scapes/visual/Fwd.h>

#include <scapes/foundation/math/Math.h>
#include <scapes/foundation/game/World.h>
#include <scapes/foundation/game/Entity.h>

struct GLFWwindow;
class ApplicationFileSystem;
class ApplicationResources;
class RenderGraph;
class ImGuiRenderer;
class SceneImporter;
class SwapChain;

/*
 */
struct ApplicationState
{
	enum
	{
		MAX_TEMPORAL_FRAMES = 16,
	};

	// Render state (synchronized with UBO)
	float lerpUserValues {0.0f};
	float userMetalness {0.0f};
	float userRoughness {0.0f};
	float currentTime {0.0f};

	// CPU state (not included in UBO)
	int currentTemporalFrame {0};
	int currentEnvironment {0};
	scapes::foundation::math::vec2 temporalSamples[MAX_TEMPORAL_FRAMES];
	bool firstFrame {true};
};

struct CameraState
{
	// Render state (synchronized with UBO)
	scapes::foundation::math::mat4 view;
	scapes::foundation::math::mat4 iview;
	scapes::foundation::math::mat4 projection;
	scapes::foundation::math::mat4 iprojection;
	scapes::foundation::math::mat4 viewOld;
	scapes::foundation::math::vec4 cameraParams;
	scapes::foundation::math::vec3 cameraPosWS;

	// CPU state (not included in UBO)
	double phi {0.0f};
	double theta {0.0f};
	double radius {2.0f};
	scapes::foundation::math::vec3 target;
};

struct InputState
{
	const double rotationSpeed {0.01};
	const double scrollSpeed {5.0};
	bool rotating {false};
	double lastMouseX {0.0};
	double lastMouseY {0.0};
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
	bool windowResized {false};
	uint32_t width {0};
	uint32_t height {0};

	SceneImporter *importer {nullptr};
	scapes::foundation::game::Entity sky_light;

	RenderGraph *render_graph {nullptr};
	ApplicationResources *application_resources {nullptr};
	ApplicationState application_state;
	ApplicationFileSystem *file_system {nullptr};
	CameraState camera_state;
	InputState input_state;

	SwapChain *swap_chain {nullptr};
	scapes::foundation::render::Device *device {nullptr};
	scapes::foundation::shaders::Compiler *compiler {nullptr};
	scapes::foundation::game::World *world {nullptr};
	scapes::foundation::resources::ResourceManager *resource_manager {nullptr};
	scapes::visual::API *visual_api {nullptr};

	// TODO: make this better
	scapes::foundation::render::UniformBuffer *camera_buffer {nullptr};
	scapes::foundation::render::BindSet *camera_bindings {nullptr};
	void *camera_gpu_data {nullptr};

	scapes::foundation::render::UniformBuffer *application_buffer {nullptr};
	scapes::foundation::render::BindSet *application_bindings {nullptr};
	void *application_gpu_data {nullptr};
};
