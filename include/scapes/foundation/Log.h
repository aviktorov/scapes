#pragma once

#include <scapes/Common.h>

namespace scapes::foundation
{
	class Log
	{
	public:
		static SCAPES_API void message(const char *format, ...);
		static SCAPES_API void warning(const char *format, ...);
		static SCAPES_API void error(const char *format, ...);
		static SCAPES_API void fatal(const char *format, ...);
	};
}
