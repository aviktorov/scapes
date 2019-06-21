#include <iostream>
#include <stdexcept>

#include "VulkanApplication.h"
#include "VulkanRenderer.h"
#include "VulkanRendererContext.h"
#include "VulkanRenderData.h"
#include "VulkanUtils.h"

#include <GLFW/glfw3.h>

/*
 */
int main(void)
{
	if (!glfwInit())
		return EXIT_FAILURE;

	try
	{
		Application app;
		app.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwTerminate();
	return EXIT_SUCCESS;
}
