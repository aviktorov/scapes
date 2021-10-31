#pragma once

#include <scapes/Common.h>

namespace scapes::foundation::io
{
	// TODO: make this full-fledged class that wraps
	//       different protocols (i.e. file://, guid://, http://, https://, etc.)
	typedef const char * URI;

	enum class SeekOrigin : uint8_t
	{
		SET = 0,
		CUR,
		END,

		MAX,
	};

	class Stream
	{
	public:
		virtual ~Stream() {}

		virtual size_t read(void *data, size_t element_size, size_t element_count) = 0;
		virtual size_t write(const void *data, size_t element_size, size_t element_count) = 0;

		virtual bool seek(uint64_t offset, SeekOrigin origin) = 0;
		virtual uint64_t tell() const = 0;
		virtual uint64_t size() const = 0;
	};

	class FileSystem
	{
	public:
		virtual ~FileSystem() {}

		virtual Stream *open(const URI &path, const char *mode) = 0;
		virtual bool close(Stream *stream) = 0;
	};
}
