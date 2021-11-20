#include "shaders/spirv/Compiler.h"

#include <scapes/foundation/Log.h>
#include <scapes/foundation/io/FileSystem.h>

#include <scapes/foundation/profiler/Profiler.h>

namespace scapes::foundation::shaders::spirv
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

				// Raytrace pipeline
				case ShaderType::RAY_GENERATION: return shaderc_raygen_shader;
				case ShaderType::INTERSECTION: return shaderc_intersection_shader;
				case ShaderType::ANY_HIT: return shaderc_anyhit_shader;
				case ShaderType::CLOSEST_HIT: return shaderc_closesthit_shader;
				case ShaderType::MISS: return shaderc_miss_shader;
				case ShaderType::CALLABLE: return shaderc_callable_shader;
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
			SCAPES_PROFILER();
			io::FileSystem *file_system = reinterpret_cast<io::FileSystem *>(user_data);

			shaderc_include_result *result = new shaderc_include_result();
			result->user_data = user_data;
			result->source_name = nullptr;
			result->source_name_length = 0;
			result->content = nullptr;
			result->content_length = 0;

			std::string target_dir = "";

			switch (type)
			{
				case shaderc_include_type_relative:
				{
					std::string_view source_path = requesting_source;
					size_t pos = source_path.find_last_of("/\\");

					if (pos != std::string_view::npos)
						target_dir = source_path.substr(0, pos + 1);
				}
				break;
			}

			std::string target_path = target_dir + std::string(requested_source);

			io::Stream *file = file_system->open(target_path.c_str(), "rb");

			if (!file)
			{
				Log::error("shaderc::include_resolver(): can't load include at \"%s\"\n", target_path);
				return result;
			}

			size_t file_size = static_cast<size_t>(file->size());
			char *buffer = new char[file_size];

			file->seek(0, io::SeekOrigin::SET);
			file->read(buffer, sizeof(char), file_size);
			file_system->close(file);

			char *path = new char[target_path.size() + 1];
			memcpy(path, target_path.c_str(), target_path.size());
			path[target_path.size()] = '\x0';

			result->source_name = path;
			result->source_name_length = target_path.size() + 1;
			result->content = buffer;
			result->content_length = file_size;

			return result;
		}

		static void includeResultReleaser(void *userData, shaderc_include_result *result)
		{
			SCAPES_PROFILER();

			delete result->source_name;
			delete result->content;
			delete result;
		}
	}

	Compiler::Compiler(io::FileSystem *file_system)
		: file_system(file_system), cache(ShaderILType::SPIRV)
	{
		compiler = shaderc_compiler_initialize();
		options = shaderc_compile_options_initialize();

		shaderc_compile_options_set_include_callbacks(options, shaderc::includeResolver, shaderc::includeResultReleaser, file_system);
		shaderc_compile_options_set_target_env(options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);

		cache.load(file_system, "spirv.cache");
	}

	Compiler::~Compiler()
	{
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);
	}

	shaders::ShaderIL *Compiler::createShaderIL(
		ShaderType type,
		uint32_t size,
		const char *data,
		const io::URI &uri
	)
	{
		SCAPES_PROFILER();

		const char *path = (uri) ? uri : "memory";

		// preprocess shader
		shaderc_compilation_result_t preprocess_result = nullptr;
		{
			SCAPES_PROFILER_N("shaders::spirv::Compiler::preprocess");
			preprocess_result = shaderc_compile_into_preprocessed_text(
				compiler,
				data, size,
				shaderc::getShaderKind(type),
				path,
				"main",
				options
			);
		}

		size_t code_length = shaderc_result_get_length(preprocess_result);
		const char *code_data = shaderc_result_get_bytes(preprocess_result);

		// check cache
		uint64_t hash = 0;
		{
			SCAPES_PROFILER_N("shaders::spirv::Compiler::check hash");
			hash = cache.getHash(type, code_data, code_length);

			ShaderIL *cache_entry = cache.get(hash);
			if (cache_entry)
			{
				shaderc_result_release(preprocess_result);
				return cache_entry;
			}
		}

		// compile preprocessed shader
		shaderc_compilation_result_t compilation_result = nullptr;
		{
			SCAPES_PROFILER_N("shaders::spirv::Compiler::compile");
			compilation_result = shaderc_compile_into_spv(
				compiler,
				code_data, code_length,
				shaderc::getShaderKind(type),
				path,
				"main",
				options
			);
		}

		shaderc_result_release(preprocess_result);

		if (shaderc_result_get_compilation_status(compilation_result) != shaderc_compilation_status_success)
		{
			Log::error("Compiler::createShaderIL(): can't compile shader at \"%s\"\n", path);
			Log::error("\t%s\n", shaderc_result_get_error_message(compilation_result));

			shaderc_result_release(compilation_result);

			return nullptr;
		}

		size_t bytecode_size = shaderc_result_get_length(compilation_result);
		const uint8_t *bytecode_data = reinterpret_cast<const uint8_t*>(shaderc_result_get_bytes(compilation_result));

		ShaderIL *result = cache.insert(hash, type, bytecode_data, bytecode_size);
		cache.flush(file_system, "spirv.cache");

		shaderc_result_release(compilation_result);

		return result;
	}

	void Compiler::releaseShaderIL(shaders::ShaderIL *il)
	{
		if (il == nullptr)
			return;

		// do nothing, because we're using shader cache
	}
}
