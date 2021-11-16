#pragma once

#ifdef SCAPES_PROFILER_ENABLED
	#include <scapes/3rdparty/tracy/common/TracyColor.hpp>
	#include <scapes/3rdparty/tracy/common/TracySystem.hpp>

	#include <scapes/3rdparty/tracy/client/TracyProfiler.hpp>
	#include <scapes/3rdparty/tracy/client/TracyScoped.hpp>

	namespace scapes::foundation
	{
		// All your base are belong to us
		namespace profiler = tracy;
	}

	#define SCAPES_CONCAT_INDIRECT(x,y) x##y
	#define SCAPES_CONCAT(x,y) SCAPES_CONCAT_INDIRECT(x,y)

	#define SCAPES_PROFILER_SCOPED_NC(name, color, active) \
		static constexpr scapes::foundation::profiler::SourceLocationData SCAPES_CONCAT(__scapes_source_line_, __LINE__) \
		{ \
			name, \
			__FUNCTION__, __FILE__, (uint32_t)__LINE__, \
			color \
		}; \
		scapes::foundation::profiler::ScopedZone SCAPES_CONCAT(__scapes_scoped_zone_, __LINE__)( \
			&SCAPES_CONCAT(__scapes_source_line_, __LINE__), \
			active \
		);

	#define SCAPES_PROFILER() SCAPES_PROFILER_SCOPED_NC(nullptr, 0, true)
	#define SCAPES_PROFILER_N(name) SCAPES_PROFILER_SCOPED_NC(name, 0, true)
	#define SCAPES_PROFILER_C(color) SCAPES_PROFILER_SCOPED_NC(nullptr, color, true)
	#define SCAPES_PROFILER_NC(name, color) SCAPES_PROFILER_SCOPED_NC(name, color, true)
#else
	#define SCAPES_PROFILER()
	#define SCAPES_PROFILER_N(name)
	#define SCAPES_PROFILER_C(color)
	#define SCAPES_PROFILER_NC(name, color)
#endif
