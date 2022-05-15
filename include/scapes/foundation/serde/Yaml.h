#pragma once

#include <scapes/3rdparty/rapidyaml/ryml_all.hpp>
#include <scapes/Common.h>
#include <scapes/foundation/math/Math.h>

namespace scapes::foundation::serde
{
	// All your base are belong to us
	namespace yaml = ryml;
}

namespace c4
{
	namespace yaml = scapes::foundation::serde::yaml;
	namespace math = scapes::foundation::math;

	/*
	 */
	using qualifier = math::qualifier;

	template<typename T, qualifier Q>
	SCAPES_INLINE bool from_chars(const yaml::csubstr buf, math::vec<2, T, Q> *vec)
	{
		size_t ret = yaml::unformat(buf, "{},{}", vec->x, vec->y);
		return ret != ryml::yml::npos;
	}

	template<typename T, qualifier Q>
	SCAPES_INLINE bool from_chars(const yaml::csubstr buf, math::vec<3, T, Q> *vec)
	{
		size_t ret = yaml::unformat(buf, "{},{},{}", vec->x, vec->y, vec->z);
		return ret != ryml::yml::npos;
	}

	template<typename T, qualifier Q>
	SCAPES_INLINE bool from_chars(const yaml::csubstr buf, math::vec<4, T, Q> *vec)
	{
		size_t ret = yaml::unformat(buf, "{},{},{},{}", vec->x, vec->y, vec->z, vec->w);
		return ret != ryml::yml::npos;
	}

	SCAPES_INLINE bool from_chars(const yaml::csubstr buf, math::mat3 *mat)
	{
		math::vec3 &c0 = (*mat)[0];
		math::vec3 &c1 = (*mat)[1];
		math::vec3 &c2 = (*mat)[2];

		size_t ret = yaml::unformat(
			buf,
			"{},{},{},{},{},{},{},{},{}",
			c0.x, c0.y, c0.z,
			c1.x, c1.y, c1.z,
			c2.x, c2.y, c2.z
		);

		return ret != ryml::yml::npos;
	}

	SCAPES_INLINE bool from_chars(const yaml::csubstr buf, math::mat4 *mat)
	{
		math::vec4 &c0 = (*mat)[0];
		math::vec4 &c1 = (*mat)[1];
		math::vec4 &c2 = (*mat)[2];
		math::vec4 &c3 = (*mat)[3];

		size_t ret = yaml::unformat(
			buf,
			"{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}",
			c0.x, c0.y, c0.z, c0.w,
			c1.x, c1.y, c1.z, c1.w,
			c2.x, c2.y, c2.z, c2.w,
			c3.x, c3.y, c3.z, c3.w
		);

		return ret != ryml::yml::npos;
	}
}
