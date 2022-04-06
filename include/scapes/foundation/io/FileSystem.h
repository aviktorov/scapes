#pragma once

#include <scapes/Common.h>
#include <cstdlib>
#include <cstring>

namespace scapes::foundation::io
{
	class URI
	{
	public:
		URI()
		{
			data[0] = '\x0';
		}
		URI(const char *str)
		{
			size_t len = strlen(str);
			assert(len < 2048);
			memcpy(data, str, sizeof(char) * (len + 1));
		}
		URI(const URI &uri)
		{
			memcpy(data, uri.data, sizeof(char) * 2048);
		}
		URI &operator=(const URI &uri)
		{
			memcpy(data, uri.data, sizeof(char) * 2048);
			return *this;
		}

		bool operator==(const URI &uri) const
		{
			return strcmp(data, uri.data) == 0;
		}

		SCAPES_INLINE const char *c_str() const { return data; }
		SCAPES_INLINE bool empty() const { return data[0] == '\0'; }

	private:
		char data[2048];
	};
	
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
		virtual uint64_t mtime(const URI &path) = 0;
	};
}
