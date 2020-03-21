#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

struct GLFWwindow;
class RenderScene;
class VulkanRenderer;
class VulkanImGuiRenderer;
class VulkanSwapChain;
class VulkanContext;

/*
 */
struct ApplicationState
{
	glm::mat4 world;
	glm::mat4 view;
	glm::mat4 proj;
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
	const double scrollSpeed {1.5};
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

	void initVulkan();
	void shutdownVulkan();

	void initVulkanSwapChain();
	void shutdownVulkanSwapChain();
	void recreateVulkanSwapChain();

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

	RenderScene *scene {nullptr};
	ApplicationState state;
	CameraState camera;
	InputState input;

	VulkanRenderer *renderer {nullptr};
	VulkanImGuiRenderer *imguiRenderer {nullptr};

	VulkanSwapChain *swapChain {nullptr};
	VulkanContext *context {nullptr};
};
