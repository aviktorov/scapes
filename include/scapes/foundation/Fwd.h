#pragma once

#include <scapes/foundation/math/Fwd.h>
#include <scapes/3rdparty/rapidjson/fwd.h>

template<typename T> struct TypeTraits;
template <typename T> struct ResourcePipeline;

namespace scapes::foundation
{
	class Log;

	namespace game
	{
		struct EntityID;
		struct QueryID;

		class World;
		class Entity;

		template<typename... Components> class Query;
	}

	namespace io
	{
		enum class SeekOrigin : uint8_t;

		class URI;
		class FileSystem;
		class Stream;
	}

	namespace json = rapidjson;
	namespace math = glm;


	namespace resources
	{
		// TODO: get rid of this once we'll have fully-fledged classes for timestamps and generations
		typedef uint32_t timestamp_t;
		typedef uint32_t generation_t;

		struct ResourceMetadata;
		template <typename T> class ResourceHandle;

		class ResourceManager;
	}
}
