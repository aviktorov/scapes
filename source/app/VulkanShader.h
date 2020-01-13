#pragma once

#include <volk.h>
#include <string>

#include "shaderc/shaderc.h"

class VulkanContext;

enum class VulkanShaderKind
{
	Vertex = 0,
	Fragment,
	Compute,
	Geometry,
	TessellationControl,
	TessellationEvaulation,
};

/*
 */
class VulkanShader
{
public:
	VulkanShader(const VulkanContext *context)
		: context(context) { }

	~VulkanShader();

	bool compileFromFile(const char *path);
	bool compileFromFile(const char *path, VulkanShaderKind kind);
	bool reload();
	void clear();

	inline VkShaderModule getShaderModule() const { return shaderModule; }

private:
	bool compileFromSourceInternal(const char *path, const char *sourceData, size_t sourceSize, shaderc_shader_kind kind);

private:
	const VulkanContext *context {nullptr};

	std::string shaderPath;
	VkShaderModule shaderModule {VK_NULL_HANDLE};
};
