#include "VulkanShader.h"
#include "VulkanUtils.h"

#include <fstream>
#include <iostream>
#include <vector>

/*
 */
static shaderc_shader_kind vulkan_to_shaderc_kind(VulkanShaderKind kind)
{
	switch(kind)
	{
		case VulkanShaderKind::Vertex: return shaderc_vertex_shader;
		case VulkanShaderKind::Fragment: return shaderc_fragment_shader;
		case VulkanShaderKind::Compute: return shaderc_compute_shader;
		case VulkanShaderKind::Geometry: return shaderc_geometry_shader;
		case VulkanShaderKind::TessellationControl: return shaderc_tess_control_shader;
		case VulkanShaderKind::TessellationEvaulation: return shaderc_tess_evaluation_shader;
	}

	throw std::runtime_error("Unsupported shader kind");
}

/*
 */
VulkanShader::~VulkanShader()
{
	clear();
}

/*
 */
bool VulkanShader::compileFromFile(const char *path)
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

	return compileFromSourceInternal(path, buffer.data(), buffer.size(), shaderc_glsl_infer_from_source);
}

bool VulkanShader::compileFromFile(const char *path, VulkanShaderKind kind)
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

	return compileFromSourceInternal(path, buffer.data(), buffer.size(), vulkan_to_shaderc_kind(kind));
}

bool VulkanShader::compileFromSourceInternal(const char *path, const char *sourceData, size_t sourceSize, shaderc_shader_kind kind)
{
	// convert glsl/hlsl code to SPIR-V bytecode
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	shaderc_compilation_result_t result = shaderc_compile_into_spv(
		compiler,
		sourceData, sourceSize,
		shaderc_glsl_infer_from_source,
		path,
		"main",
		nullptr
	);

	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success)
	{
		std::cerr << "VulkanShader::loadFromFile(): can't compile shader at \"" << path << "\"" << std::endl;
		std::cerr << "\t" << shaderc_result_get_error_message(result);

		shaderc_result_release(result);
		shaderc_compiler_release(compiler);

		return false;
	}

	size_t size = shaderc_result_get_length(result);
	const uint32_t *data = reinterpret_cast<const uint32_t*>(shaderc_result_get_bytes(result));

	clear();
	shaderModule = VulkanUtils::createShaderModule(context, data, size);

	shaderc_result_release(result);
	shaderc_compiler_release(compiler);

	return true;
}

void VulkanShader::clear()
{
	vkDestroyShaderModule(context.device, shaderModule, nullptr);
	shaderModule = VK_NULL_HANDLE;
}
