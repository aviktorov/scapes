#pragma once

#include <cstdint>

namespace io
{
	enum class SeekOrigin : uint8_t
	{
		SET = 0,
		CUR,
		END,

		MAX,
	};

	class IStream
	{
	public:
		virtual ~IStream() {}

		virtual size_t read(void *data, size_t element_size, size_t element_count) = 0;
		virtual size_t write(const void *data, size_t element_size, size_t element_count) = 0;

		virtual bool seek(uint64_t offset, SeekOrigin origin) = 0;
		virtual uint64_t tell() const = 0;
		virtual uint64_t size() const = 0;
	};

	class IFileSystem
	{
	public:
		virtual ~IFileSystem() {}

		virtual IStream *open(const char *path, const char *mode) = 0;
		virtual bool close(IStream *stream) = 0;
	};
}
