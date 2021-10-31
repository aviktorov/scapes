#pragma once

#include <scapes/foundation/math/Math.h>
#include <scapes/foundation/TypeTraits.h>

namespace scapes::foundation::components
{
	/*
	 */
	struct Transform
	{
		math::mat4 transform;
	};

	template<>
	struct ::TypeTraits<Transform>
	{
		static constexpr const char *name = "scapes::foundation::components::Transform";
	};
}
