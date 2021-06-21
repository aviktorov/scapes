#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

namespace render
{
	class SwapChain;
}

namespace render::shaders
{
	class Compiler;
}

namespace render::backend
{
	class Driver;
	struct UniformBuffer;
	struct BindSet;
}

struct GLFWwindow;
class ApplicationResources;
class RenderGraph;
class Renderer;
class ImGuiRenderer;
class Scene;
class SkyLight;

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
	glm::vec2 temporalSamples[MAX_TEMPORAL_FRAMES];
	bool firstFrame {true};
};

struct CameraState
{
	// Render state (synchronized with UBO)
	glm::mat4 view;
	glm::mat4 iview;
	glm::mat4 projection;
	glm::mat4 iprojection;
	glm::mat4 viewOld;
	glm::vec4 cameraParams;
	glm::vec3 cameraPosWS;

	// CPU state (not included in UBO)
	double phi {0.0f};
	double theta {0.0f};
	double radius {2.0f};
	glm::vec3 target;
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

	Scene *sponza {nullptr};
	SkyLight *sky_light {nullptr};

	RenderGraph *render_graph {nullptr};
	ApplicationResources *resources {nullptr};
	ApplicationState application_state;
	CameraState camera_state;
	InputState input_state;

	render::SwapChain *swap_chain {nullptr};
	render::backend::Driver *driver {nullptr};
	render::shaders::Compiler *compiler {nullptr};

	render::backend::UniformBuffer *camera_buffer {nullptr};
	render::backend::BindSet *camera_bindings {nullptr};
	void *camera_gpu_data {nullptr};
};
