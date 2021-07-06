#include "render/shaders/spirv/Compiler.h"

#include <shaderc/shaderc.h>
#include <iostream>
#include <fstream>
#include <cassert>

namespace render::shaders::spirv
{
	namespace shaderc
	{
		static shaderc_shader_kind getShaderKind(ShaderType type)
		{
			switch(type)
			{
				// Graphics pipeline
				case ShaderType::VERTEX: return shaderc_vertex_shader;
				case ShaderType::TESSELLATION_CONTROL: return shaderc_tess_control_shader;
				case ShaderType::TESSELLATION_EVALUATION: return shaderc_tess_evaluation_shader;
				case ShaderType::GEOMETRY: return shaderc_geometry_shader;
				case ShaderType::FRAGMENT: return shaderc_fragment_shader;

				// Compute pipeline
				case ShaderType::COMPUTE: return shaderc_compute_shader;

#if NV_EXTENSIONS
				// Raytrace pipeline
				case ShaderType::RAY_GENERATION: return shaderc_raygen_shader;
				case ShaderType::INTERSECTION: return shaderc_intersection_shader;
				case ShaderType::ANY_HIT: return shaderc_anyhit_shader;
				case ShaderType::CLOSEST_HIT: return shaderc_closesthit_shader;
				case ShaderType::MISS: return shaderc_miss_shader;
				case ShaderType::CALLABLE: return shaderc_callable_shader;
#endif
			}

			return shaderc_glsl_infer_from_source;
		}

		static shaderc_include_result *includeResolver(
			void *user_data,
			const char *requested_source,
			int type,
			const char *requesting_source,
			size_t include_depth
		)
		{
			shaderc_include_result *result = new shaderc_include_result();
			result->user_data = user_data;
			result->source_name = nullptr;
			result->source_name_length = 0;
			result->content = nullptr;
			result->content_length = 0;

			std::string target_dir = "assets/";

			switch (type)
			{
				case shaderc_include_type_standard:
				{
					// TODO: remove this, not generic
					target_dir += "shaders/";
				}
				break;

				case shaderc_include_type_relative:
				{
					std::string_view source_path = requesting_source;
					size_t pos = source_path.find_last_of("/\\");

					if (pos != std::string_view::npos)
						target_dir += source_path.substr(0, pos + 1);
				}
				break;
			}

			std::string target_path = target_dir + std::string(requested_source);

			std::ifstream file(target_path, std::ios::ate | std::ios::binary);

			if (!file.is_open())
			{
				std::cerr << "shaderc::include_resolver(): can't load include at \"" << target_path << "\"" << std::endl;
				return result;
			}

			size_t fileSize = static_cast<size_t>(file.tellg());
			char *buffer = new char[fileSize];

			file.seekg(0);
			file.read(buffer, fileSize);
			file.close();

			char *path = new char[target_path.size() + 1];
			memcpy(path, target_path.c_str(), target_path.size());
			path[target_path.size()] = '\x0';

			result->source_name = path;
			result->source_name_length = target_path.size() + 1;
			result->content = buffer;
			result->content_length = fileSize;

			return result;
		}

		static void includeResultReleaser(void *userData, shaderc_include_result *result)
		{
			delete result->source_name;
			delete result->content;
			delete result;
		}
	}

	shaders::ShaderIL *Compiler::createShaderIL(
		ShaderType type,
		uint32_t size,
		const char *data,
		const char *path
	)
	{
		if (path == nullptr)
			path = "memory";

		// convert GLSL/HLSL code to SPIR-V bytecode
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();

		// set compile options
		shaderc_compile_options_set_include_callbacks(options, shaderc::includeResolver, shaderc::includeResultReleaser, nullptr);

		// compile shader
		shaderc_compilation_result_t compilation_result = shaderc_compile_into_spv(
			compiler,
			data, size,
			shaderc_glsl_infer_from_source,
			path,
			"main",
			options
		);

		if (shaderc_result_get_compilation_status(compilation_result) != shaderc_compilation_status_success)
		{
			std::cerr << "Compiler::createShaderIL(): can't compile shader at \"" << path << "\"" << std::endl;
			std::cerr << "\t" << shaderc_result_get_error_message(compilation_result);

			shaderc_result_release(compilation_result);
			shaderc_compile_options_release(options);
			shaderc_compiler_release(compiler);

			return nullptr;
		}

		size_t bytecode_size = shaderc_result_get_length(compilation_result);
		const uint32_t *bytecode_data = reinterpret_cast<const uint32_t*>(shaderc_result_get_bytes(compilation_result));

		// Copy bytecode to ShaderIL
		ShaderIL *result = new ShaderIL();
		result->bytecode_size = bytecode_size;
		result->bytecode_data = new uint32_t[bytecode_size];
		result->type = type;
		result->il_type = ShaderILType::SPIRV;

		memcpy(result->bytecode_data, bytecode_data, bytecode_size);

		shaderc_result_release(compilation_result);
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		return result;
	}

	void Compiler::destroyShaderIL(shaders::ShaderIL *il)
	{
		if (il == nullptr)
			return;

		ShaderIL *spirv_shader_il = static_cast<ShaderIL *>(il);

		delete[] spirv_shader_il->bytecode_data;
		spirv_shader_il->bytecode_data = nullptr;
		spirv_shader_il->bytecode_size = 0;

		delete spirv_shader_il;
	}
}
