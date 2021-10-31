#pragma once

#include <scapes/foundation/game/World.h>

namespace scapes::foundation::game
{
	template<typename... Components>
	class Query
	{
	public:
		Query() : world(nullptr), query(nullptr)
		{

		}

		Query(World *world) : world(world)
		{
			constexpr size_t num_components = sizeof...(Components);
			const char *names[num_components];
			size_t sizes[num_components];
			size_t alignments[num_components];

			collect_args(0, names, sizes, alignments, (typename std::decay<Components>::type*)nullptr...);

			query = world->createQuery(num_components, names, sizes, alignments);
		}

		~Query()
		{
			world->destroyQuery(query);
			query = nullptr;
		}

		inline bool begin()
		{
			return world->begin(query);
		}

		inline bool next()
		{
			return world->next(query);
		}

		inline uint32_t getNumComponents()
		{
			return world->getNumQueryComponents(query);
		}

		template<typename T>
		inline T *getComponents(uint32_t index)
		{
			return reinterpret_cast<T*>(world->getQueryComponents(query, index));
		}

	private:
		template<typename T, typename... Components>
		inline void collect_args(int index, const char *names[], size_t sizes[], size_t alignments[], T comp, Components... comps)
		{
			using Component = typename std::remove_pointer<T>::type;
			names[index] = TypeTraits<Component>::name;
			sizes[index] = sizeof(Component);
			alignments[index] = alignof(Component);

			if constexpr (sizeof...(Components) != 0)
				collect_args(index + 1, names, sizes, alignments, comps...);
		}

	private:
		World *world {nullptr};
		QueryID *query {nullptr};
	};
}
