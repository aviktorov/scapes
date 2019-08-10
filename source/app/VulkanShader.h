#pragma once

#include <volk.h>
#include <string>

#include "VulkanRendererContext.h"

/*
 */
class VulkanShader
{
public:
	VulkanShader(const VulkanRendererContext &context)
		: context(context) { }

	~VulkanShader();

	bool loadFromFile(const std::string &path);
	void clear();

	inline VkShaderModule getShaderModule() const { return shaderModule; }

private:
	VulkanRendererContext context;

	VkShaderModule shaderModule {VK_NULL_HANDLE};
};
