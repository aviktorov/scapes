#pragma once

#include <scapes/foundation/game/World.h>

namespace scapes::foundation::game
{
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
}
