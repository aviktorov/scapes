#pragma once

#include <scapes/Common.h>
#include <scapes/foundation/TypeTraits.h>

namespace scapes::foundation::game
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
}
