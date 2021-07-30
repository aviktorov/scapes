#pragma once

#include <game/World.h>
#include <flecs.h>
#include <unordered_map>
#include <string>

namespace game::flecs
{
	struct QueryID : public game::QueryID
	{
		::flecs::query_t *query {nullptr};
		uint32_t num_types {0};
		std::vector<size_t> type_sizes;
		ecs_iter_t iter {0};
	};

	class World : public game::World
	{
	public:
		World();
		~World() final;

		game::QueryID *createQuery(uint32_t num_types, const char *type_names[], size_t type_sizes[], size_t type_alignments[]) final;
		void destroyQuery(game::QueryID *query) final;
		bool begin(game::QueryID *query) const final;
		bool next(game::QueryID *query) const final;
		uint32_t getNumQueryComponents(game::QueryID *query) const final;
		void *getQueryComponents(game::QueryID *query, uint32_t type_index) const final;

		game::EntityID *createEntity() final;
		void destroyEntity(game::EntityID *entity) final;
		void clear() final;

		void *addComponent(game::EntityID *entity, const char *type_name, size_t type_size, size_t type_alignment, const void *data) final;
		void *getComponent(EntityID *entity, const char *type_name, size_t type_size, size_t type_alignment) const final;

	private:
		std::string wrapTypeName(const char *type_name) const;
		::flecs::entity_t fetchComponentID(const char *type_name, size_t size, size_t alignment) const;

	private:
		::flecs::world world;
		mutable std::unordered_map<uint64_t, ::flecs::entity_t> registered_components;
	};
}
