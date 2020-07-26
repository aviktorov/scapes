#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

namespace render
{
	class SwapChain;
}

namespace render::backend
{
	class Driver;
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
	glm::mat4 world;
	glm::mat4 view;
	glm::mat4 invView;
	glm::mat4 proj;
	glm::mat4 invProj;
	glm::vec4 cameraParams;
	glm::vec3 cameraPosWS;
	float lerpUserValues {0.0f};
	float userMetalness {0.0f};
	float userRoughness {0.0f};
	int currentEnvironment {0};
};

struct CameraState
{
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
	ApplicationState state;
	CameraState camera;
	InputState input;

	render::SwapChain *swap_chain {nullptr};
	render::backend::Driver *driver {nullptr};
};
