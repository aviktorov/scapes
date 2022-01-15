#pragma once

#ifdef _WIN32
	#define SCAPES_EXPORT __declspec(dllexport)
	#define SCAPES_IMPORT __declspec(dllimport)
	#define SCAPES_INLINE __forceinline
#else
	#define SCAPES_EXPORT __attribute__((visibility("default")))
	#define SCAPES_IMPORT 
	#define SCAPES_INLINE __inline__
#endif

#if defined(SCAPES_SHARED_LIBRARY)
	#define SCAPES_API SCAPES_EXPORT
#else
	#define SCAPES_API SCAPES_IMPORT
#endif

#define SCAPES_NULL_HANDLE nullptr

#include <cassert>
#include <cstdint>
