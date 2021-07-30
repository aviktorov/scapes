#pragma once

#include "World.h"
#include <algorithm>

namespace game::flecs
{
	template <class T>
	static void hashCombine(uint64_t &s, const T &v)
	{
		std::hash<T> h;
		s^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
	}

	/*
	 */
	World::World()
	{
	}

	World::~World()
	{
		clear();

		for(auto it : registered_components)
			ecs_delete(world.c_ptr(), it.second);

		registered_components.clear();
	}

	/*
	 */
	game::QueryID *World::createQuery(uint32_t num_types, const char *type_names[], size_t type_sizes[], size_t type_alignments[])
	{
		QueryID *result = new QueryID();

		for (uint32_t i = 0; i < num_types; ++i)
			fetchComponentID(type_names[i], type_sizes[i], type_alignments[i]);

		std::stringstream ss;
		ss << wrapTypeName(type_names[0]);
		for (uint32_t i = 1; i < num_types; ++i)
			ss << ", " << wrapTypeName(type_names[i]);

		const std::string &expression = ss.str();

		result->query = ecs_query_new(world.c_ptr(), expression.c_str());
		result->type_sizes.resize(num_types);
		result->num_types = num_types;

		memcpy(result->type_sizes.data(), type_sizes, sizeof(size_t) * num_types);

		return result;
	}

	void World::destroyQuery(game::QueryID *query)
	{
		QueryID *flecs_query = static_cast<QueryID *>(query);

		ecs_query_free(flecs_query->query);
		delete flecs_query;
	}

	bool World::begin(game::QueryID *query) const
	{
		QueryID *flecs_query = static_cast<QueryID *>(query);

		flecs_query->iter = ecs_query_iter(flecs_query->query);

		return true;
	}

	bool World::next(game::QueryID *query) const
	{
		QueryID *flecs_query = static_cast<QueryID *>(query);

		return ecs_query_next(&flecs_query->iter);
	}

	uint32_t World::getNumQueryComponents(game::QueryID *query) const
	{
		QueryID *flecs_query = static_cast<QueryID *>(query);

		return flecs_query->iter.count;
	}

	void *World::getQueryComponents(game::QueryID *query, uint32_t type_index) const
	{
		QueryID *flecs_query = static_cast<QueryID *>(query);

		return ecs_column_w_size(&flecs_query->iter, flecs_query->type_sizes[type_index], type_index + 1);
	}

	/*
	 */
	game::EntityID *World::createEntity()
	{
		::flecs::entity entity = world.entity();
		::flecs::entity_t id = entity.id();

		return reinterpret_cast<game::EntityID*>(id);
	}

	void World::destroyEntity(game::EntityID *entity)
	{
		::flecs::entity_t id = reinterpret_cast<::flecs::entity_t>(entity);
		ecs_delete(world.c_ptr(), id);
	}

	void World::clear()
	{
		world.delete_entities(::flecs::filter());
	}

	void *World::addComponent(game::EntityID *entity, const char *type_name, size_t type_size, size_t type_alignment, const void *data)
	{
		assert(type_name);
		assert(type_size > 0);

		::flecs::entity_t id = reinterpret_cast<::flecs::entity_t>(entity);
		::flecs::entity_t comp_id = fetchComponentID(type_name, type_size, type_alignment);

		assert(id != 0);
		assert(comp_id != 0);

		::flecs::entity_t result_id = ecs_set_ptr_w_entity(world.c_ptr(), id, comp_id, type_size, data);
		assert(result_id == id);

		bool is_added = false;

		void *result = ecs_get_mut_w_entity(world.c_ptr(), id, comp_id, &is_added);
		assert(is_added == false);

		return result;
	}

	void *World::getComponent(game::EntityID *entity, const char *type_name, size_t type_size, size_t type_alignment) const
	{
		assert(type_name);
		assert(type_size > 0);

		::flecs::entity_t id = reinterpret_cast<::flecs::entity_t>(entity);
		::flecs::entity_t comp_id = fetchComponentID(type_name, type_size, type_alignment);

		assert(id != 0);
		assert(comp_id != 0);

		bool is_added = false;

		void *result = ecs_get_mut_w_entity(world.c_ptr(), id, comp_id, &is_added);
		assert(is_added == false);

		return result;
	}

	/*
	 */
	std::string World::wrapTypeName(const char *type_name) const
	{
		std::string result(type_name);
		std::replace(result.begin(), result.end(), ':', '_'); // WTF flecs

		return result;
	}

	::flecs::entity_t World::fetchComponentID(const char *type_name, size_t size, size_t alignment) const
	{
		const std::string &name = wrapTypeName(type_name);

		uint64_t hash = 0;
		hashCombine(hash, name);
		hashCombine(hash, size);
		hashCombine(hash, alignment);

		auto it = registered_components.find(hash);
		if (it != registered_components.end())
			return it->second;

		::flecs::entity_t result = ecs_new_component(world.c_ptr(), 0, name.c_str(), size, alignment);
		registered_components[hash] = result;

		return result;
	}
}
