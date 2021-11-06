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

	#define SCAPES_CONCAT(x,y) TracyConcatIndirect(x,y)

	#define SCAPES_PROFILER_ZONE_NC(varname, name, color, active) \
		static constexpr scapes::foundation::profiler::SourceLocationData SCAPES_CONCAT(__scapes_profiler_source_location, __LINE__) \
		{ \
			name, \
			__FUNCTION__, __FILE__, (uint32_t)__LINE__, \
			color \
		}; \
		scapes::foundation::profiler::ScopedZone varname( \
			&SCAPES_CONCAT(__scapes_profiler_source_location, __LINE__), \
			active \
		);

	#define SCAPES_PROFILER_NAMED(zone) SCAPES_PROFILER_ZONE_NC(zone, nullptr, 0, true)
	#define SCAPES_PROFILER_NAMED_N(zone, name) SCAPES_PROFILER_ZONE_NC(zone, name, 0, true)
	#define SCAPES_PROFILER_NAMED_C(zone, color) SCAPES_PROFILER_ZONE_NC(zone, nullptr, color, true)
	#define SCAPES_PROFILER_NAMED_NC(zone, name, color) SCAPES_PROFILER_ZONE_NC(zone, name, color, true)

	#define SCAPES_PROFILER_SCOPED SCAPES_PROFILER_NAMED(___scapes_scoped_zone)
	#define SCAPES_PROFILER_SCOPED_N(name) SCAPES_PROFILER_NAMED_N(___scapes_scoped_zone, name)
	#define SCAPES_PROFILER_SCOPED_C(color) SCAPES_PROFILER_NAMED_C(___scapes_scoped_zone, color)
	#define SCAPES_PROFILER_SCOPED_NC(name, color) SCAPES_PROFILER_NAMED_NC(___scapes_scoped_zone, name, color)
#else
	#define SCAPES_PROFILER_SCOPED
	#define SCAPES_PROFILER_SCOPED_N(name)
	#define SCAPES_PROFILER_SCOPED_C(color)
	#define SCAPES_PROFILER_SCOPED_NC(name, color)

	#define SCAPES_PROFILER_NAMED(zone)
	#define SCAPES_PROFILER_NAMED_N(zone, name)
	#define SCAPES_PROFILER_NAMED_C(zone, color)
	#define SCAPES_PROFILER_NAMED_NC(zone, name, color)
#endif
