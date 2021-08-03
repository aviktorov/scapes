#pragma once

#include <common/Common.h>
#include <common/Type.h>

namespace game
{
	struct EntityID {};
	struct QueryID {};

	class World
	{
	public:
		static SCAPES_API World *create();
		static SCAPES_API void destroy(World *world);

		virtual ~World() {}

		virtual QueryID *createQuery(uint32_t num_types, const char *type_names[], size_t type_sizes[], size_t type_alignments[]) = 0;
		virtual void destroyQuery(QueryID *query) = 0;
		virtual bool begin(QueryID *query) const = 0;
		virtual bool next(QueryID *query) const = 0;
		// TODO: better names
		virtual uint32_t getNumQueryComponents(QueryID *query) const = 0;
		virtual void *getQueryComponents(QueryID *query, uint32_t type_index) const = 0;

		virtual EntityID *createEntity() = 0;
		virtual void destroyEntity(EntityID *entity) = 0;
		virtual void clear() = 0;

		virtual void *addComponent(EntityID *entity, const char *type_name, size_t type_size, size_t type_alignment, const void *data) = 0;
		virtual void *getComponent(EntityID *entity, const char *type_name, size_t type_size, size_t type_alignment) const = 0;
	};

	class Entity
	{
	public:
		Entity() : world(nullptr), id(nullptr)
		{

		}

		Entity(World *world) : world(world)
		{
			id = world->createEntity();
		}

		inline EntityID *getID() const { return id; }

		template<typename T>
		inline T &getComponent()
		{
			void *comp = world->getComponent(id, TypeTraits<T>::name, sizeof(T), alignof(T));
			assert(comp);

			return *reinterpret_cast<T*>(comp);
		}

		template<typename T>
		inline const T &getComponent() const
		{
			void *comp = world->getComponent(id, TypeTraits<T>::name, sizeof(T), alignof(T));
			assert(comp);

			return *reinterpret_cast<T*>(comp);
		}

		template<typename T>
		inline T &addComponent()
		{
			void *comp = world->addComponent(id, TypeTraits<T>::name, sizeof(T), alignof(T), nullptr);
			assert(comp);

			return *reinterpret_cast<T*>(comp);
		}

		template<typename T, typename... Arguments>
		inline T &addComponent(Arguments&&... params)
		{
			const T &temp = { std::forward<Arguments>(params)... };
			void *comp = world->addComponent(id, TypeTraits<T>::name, sizeof(T), alignof(T), reinterpret_cast<const void *>(&temp));
			assert(comp);

			return *reinterpret_cast<T*>(comp);
		}

	private:
		World *world {nullptr};
		EntityID *id {nullptr};
	};

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
