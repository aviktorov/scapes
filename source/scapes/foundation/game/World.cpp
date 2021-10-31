#include <game/flecs/World.h>

namespace scapes::foundation::game
{
	World *World::create()
	{
		return new flecs::World();
	}

	void World::destroy(World *world)
	{
		delete world;
	}
}
