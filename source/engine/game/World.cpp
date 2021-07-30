#include <game/World.h>
#include <game/flecs/World.h>

namespace game
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
