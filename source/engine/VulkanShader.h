// TODO: remove Vulkan dependencies
#pragma once

#include <volk.h>

#include <string>
#include <render/backend/driver.h>

/*
 */
class VulkanShader
{
public:
	VulkanShader(render::backend::Driver *driver)
		: driver(driver) { }

	~VulkanShader();

	bool compileFromFile(const char *path, render::backend::ShaderType type);
	bool reload();
	void clear();

	VkShaderModule getShaderModule() const;

private:
	render::backend::Driver *driver {nullptr};
	render::backend::Shader *shader {nullptr};
	render::backend::ShaderType type {render::backend::ShaderType::FRAGMENT};

	std::string path;
};
