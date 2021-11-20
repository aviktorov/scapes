#include "shaders/ShaderCache.h"
#include "HashUtils.h"

#include <scapes/foundation/io/FileSystem.h>

namespace scapes::foundation::shaders
{
	ShaderCache::~ShaderCache()
	{
		clear();
	}

	bool ShaderCache::load(io::FileSystem *file_system, const io::URI &cache_path)
	{
		io::Stream *cache_file = file_system->open(cache_path, "rb");

		if (!cache_file)
			return false;

		while (true)
		{
			uint64_t hash = 0;
			ShaderType shader_type = ShaderType::FRAGMENT;
			uint64_t size = 0;
			size_t bytes_read = 0;

			bytes_read = cache_file->read(&hash, sizeof(uint64_t), 1);
			if (bytes_read != 1)
				break;

			bytes_read = cache_file->read(&shader_type, sizeof(ShaderType), 1);
			if (bytes_read != 1)
			{
				// TODO: log warning, cache corrupted
				file_system->close(cache_file);
				return false;
			}

			bytes_read = cache_file->read(&size, sizeof(uint64_t), 1);
			if (bytes_read != 1)
			{
				// TODO: log warning, cache corrupted
				file_system->close(cache_file);
				return false;
			}

			uint8_t *bytecode_data = new uint8_t[size];
			bytes_read = cache_file->read(bytecode_data, sizeof(uint8_t), size);
			if (bytes_read != size)
			{
				// TODO: log warning, cache corrupted
				file_system->close(cache_file);
				return false;
			}

			insert(hash, shader_type, bytecode_data, static_cast<size_t>(size));
		}

		file_system->close(cache_file);

		nonflushed_entries.clear();
		return true;
	}

	bool ShaderCache::flush(io::FileSystem *file_system, const io::URI &cache_path)
	{
		io::Stream *cache_file = file_system->open(cache_path, "ab");
		if (!cache_file)
			return false;

		for (uint64_t hash : nonflushed_entries)
		{
			ShaderIL *entry = cache[hash];
			uint64_t bytecode_size = entry->bytecode_size;

			cache_file->write(&hash, sizeof(uint64_t), 1);
			cache_file->write(&entry->type, sizeof(ShaderType), 1);
			cache_file->write(&bytecode_size, sizeof(uint64_t), 1);
			cache_file->write(entry->bytecode_data, sizeof(uint8_t), bytecode_size);
		}

		file_system->close(cache_file);

		nonflushed_entries.clear();
		return true;
	}

	uint64_t ShaderCache::getHash(ShaderType type, const char *source, size_t length) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, type);
		common::HashUtils::combine(hash, std::string_view(source, length));

		return hash;
	}

	ShaderIL *ShaderCache::get(uint64_t hash) const
	{
		auto it = cache.find(hash);

		if (it == cache.end())
			return nullptr;

		return it->second;
	}

	ShaderIL *ShaderCache::insert(uint64_t hash, ShaderType type, const uint8_t *bytecode, size_t size)
	{
		ShaderIL *cache_entry = new ShaderIL();
		cache_entry->bytecode_size = size;
		cache_entry->bytecode_data = new uint8_t[size];
		cache_entry->type = type;
		cache_entry->il_type = il_type;

		memcpy(cache_entry->bytecode_data, bytecode, size);

		cache.insert({hash, cache_entry});
		nonflushed_entries.push_back(hash);

		return cache_entry;
	}

	void ShaderCache::clear()
	{
		for (auto it : cache)
		{
			delete[] it.second->bytecode_data;
			delete it.second;
		}

		cache.clear();

		// TODO: log warning if we have non-flushed entries
		nonflushed_entries.clear();
	}
}
