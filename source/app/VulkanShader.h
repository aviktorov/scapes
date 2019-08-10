#pragma once

#include <volk.h>
#include <string>

#include "VulkanRendererContext.h"

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
	VulkanShader(const VulkanRendererContext &context)
		: context(context) { }

	~VulkanShader();

	bool compileFromFile(const std::string &path, VulkanShaderKind kind);
	void clear();

	inline VkShaderModule getShaderModule() const { return shaderModule; }

private:
	VulkanRendererContext context;

	VkShaderModule shaderModule {VK_NULL_HANDLE};
};
