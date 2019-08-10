#include "VulkanShader.h"
#include "VulkanUtils.h"

#include <fstream>
#include <iostream>
#include <vector>

/*
 */
VulkanShader::~VulkanShader()
{
	clear();
}

/*
 */
bool VulkanShader::loadFromFile(const std::string &path)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		std::cerr << "VulkanShader::loadFromFile(): can't load shader at \"" << path << "\"" << std::endl;
		return false;
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	// TODO: convert glsl/hlsl code to SPIR-V bytecode

	clear();
	shaderModule = VulkanUtils::createShaderModule(context, reinterpret_cast<uint32_t *>(buffer.data()), buffer.size());

	return true;
}

void VulkanShader::clear()
{
	vkDestroyShaderModule(context.device, shaderModule, nullptr);
	shaderModule = VK_NULL_HANDLE;
}
