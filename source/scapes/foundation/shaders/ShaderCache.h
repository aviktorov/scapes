#pragma once

#include <scapes/foundation/shaders/Compiler.h>

#include <unordered_map>
#include <vector>

namespace scapes::foundation::shaders
{
	class ShaderCache
	{
	public:
		ShaderCache(ShaderILType il_type)
			: il_type(il_type) { }

		~ShaderCache();

		bool load(io::FileSystem *file_system, const io::URI &cache_path);
		bool flush(io::FileSystem *file_system, const io::URI &cache_path);

		uint64_t getHash(ShaderType type, const char *source, size_t length) const;

		ShaderIL *get(uint64_t hash) const;
		ShaderIL *insert(uint64_t hash, ShaderType type, const uint8_t *bytecode, size_t size);
		void clear();

	private:
		ShaderILType il_type {ShaderILType::SPIRV};

		std::unordered_map<uint64_t, ShaderIL*> cache;
		std::vector<uint64_t> nonflushed_entries;
	};
}
