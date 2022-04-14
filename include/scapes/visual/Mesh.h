#pragma once

#include <scapes/foundation/TypeTraits.h>
#include <scapes/foundation/math/Math.h>
#include <scapes/foundation/resources/ResourceManager.h>

#include <scapes/visual/Fwd.h>

namespace scapes::visual
{
	/*
	 */
	struct Mesh
	{
		struct Vertex
		{
			foundation::math::vec3 position;
			foundation::math::vec4 tangent;
			foundation::math::vec3 binormal;
			foundation::math::vec3 normal;
			foundation::math::vec4 color;
			foundation::math::vec2 uv;
		};

		uint32_t num_vertices {0};
		Vertex *vertices {nullptr};

		uint32_t num_indices {0};
		uint32_t *indices {nullptr};

		hardware::VertexBuffer vertex_buffer {SCAPES_NULL_HANDLE};
		hardware::IndexBuffer index_buffer {SCAPES_NULL_HANDLE};
		hardware::Device *device {nullptr};
	};

	template <>
	struct ::TypeTraits<Mesh>
	{
		static constexpr const char *name = "scapes::visual::Mesh";
	};

	template <>
	struct ::ResourceTraits<Mesh>
	{
		static SCAPES_API size_t size();
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			hardware::Device *device
		);
		static SCAPES_API void create(
			foundation::resources::ResourceManager *resource_manager,
			void *memory,
			hardware::Device *device,
			uint32_t num_vertices,
			Mesh::Vertex *vertices,
			uint32_t num_indices,
			uint32_t *indices
		);
		static SCAPES_API foundation::resources::hash_t fetchHash(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API void flushToGPU(
			foundation::resources::ResourceManager *resource_manager,
			void *memory
		);
		static SCAPES_API bool reload(
			foundation::resources::ResourceManager *resource_manager,
			foundation::io::FileSystem *file_system,
			void *memory,
			const foundation::io::URI &uri
		);
		static SCAPES_API void destroy(
			foundation::resources::ResourceManager *resource_manager,
			void *memory
		);
	};
}
