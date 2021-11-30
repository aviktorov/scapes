#include <scapes/foundation/Log.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace scapes::foundation
{
	void Log::message(const char *format, ...)
	{
		va_list args;
		va_start(args, format);
		vfprintf(stdout, format, args);
		va_end(args);
		flushall();
	}

	void Log::warning(const char *format, ...)
	{
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
	}

	void Log::error(const char *format, ...)
	{
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
	}

	void Log::fatal(const char *format, ...)
	{
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);

		exit(EXIT_FAILURE);
	}
}
