#include "VulkanShader.h"
#include "VulkanContext.h"
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
static shaderc_include_result *vulkan_include_resolver(
	void *userData,
	const char *requestedSource,
	int type,
	const char *requestingSource,
	size_t includeDepth
)
{
	shaderc_include_result *result = new shaderc_include_result();
	result->user_data = userData;
	result->source_name = nullptr;
	result->source_name_length = 0;
	result->content = nullptr;
	result->content_length = 0;

	std::string targetDir = "";

	switch (type)
	{
		case shaderc_include_type_standard:
		{
			targetDir = "shaders/";
		}
		break;

		case shaderc_include_type_relative:
		{
			std::string_view sourcePath = requestingSource;
			size_t pos = sourcePath.find_last_of("/\\");

			if (pos != std::string_view::npos)
				targetDir = sourcePath.substr(0, pos + 1);
		}
		break;
	}

	std::string targetPath = targetDir + std::string(requestedSource);

	std::ifstream file(targetPath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		std::cerr << "vulkan_include_resolver(): can't load include at \"" << targetPath << "\"" << std::endl;
		return result;
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	char *buffer = new char[fileSize];

	file.seekg(0);
	file.read(buffer, fileSize);
	file.close();

	char *path = new char[targetPath.size() + 1];
	memcpy(path, targetPath.c_str(), targetPath.size());
	path[targetPath.size()] = '\x0';

	result->source_name = path;
	result->source_name_length = targetPath.size() + 1;
	result->content = buffer;
	result->content_length = fileSize;

	return result;
}

static void vulkan_include_result_releaser(void *userData, shaderc_include_result *result)
{
	delete result->source_name;
	delete result->content;
	delete result;
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
	shaderc_compile_options_t options = shaderc_compile_options_initialize();

	// set compile options
	shaderc_compile_options_set_include_callbacks(options, vulkan_include_resolver, vulkan_include_result_releaser, nullptr);

	// compile shader
	shaderc_compilation_result_t result = shaderc_compile_into_spv(
		compiler,
		sourceData, sourceSize,
		shaderc_glsl_infer_from_source,
		path,
		"main",
		options
	);

	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success)
	{
		std::cerr << "VulkanShader::loadFromFile(): can't compile shader at \"" << path << "\"" << std::endl;
		std::cerr << "\t" << shaderc_result_get_error_message(result);

		shaderc_result_release(result);
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		return false;
	}

	size_t size = shaderc_result_get_length(result);
	const uint32_t *data = reinterpret_cast<const uint32_t*>(shaderc_result_get_bytes(result));

	clear();
	shaderModule = VulkanUtils::createShaderModule(context, data, size);

	shaderc_result_release(result);
	shaderc_compile_options_release(options);
	shaderc_compiler_release(compiler);

	shaderPath = path;

	return true;
}

bool VulkanShader::reload()
{
	return compileFromFile(shaderPath.c_str());
}

void VulkanShader::clear()
{
	vkDestroyShaderModule(context->getDevice(), shaderModule, nullptr);
	shaderModule = VK_NULL_HANDLE;
}
