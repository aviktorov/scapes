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

private:
	// TODO: move to another class (Window?)
	GLFWwindow *window {nullptr};
	bool windowResized {false};

	RenderScene *scene {nullptr};
	ApplicationState state;

	VulkanRenderer *renderer {nullptr};
	VulkanImGuiRenderer *imguiRenderer {nullptr};

	VulkanSwapChain *swapChain {nullptr};
	VulkanContext *context {nullptr};
};
