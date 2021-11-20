#pragma once

#include <algorithm>

namespace scapes::common
{
	class HashUtils
	{
	public:
		template <class T>
		static void combine(uint64_t &s, const T &v)
		{
			std::hash<T> h;
			s^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
		}
	};
}
