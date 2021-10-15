#pragma once

#include <common/ResourceManager.h>
#include <GLM/glm.hpp>

struct aiMesh;
struct cgltf_mesh;

namespace render::backend
{
	class Driver;
	struct VertexBuffer;
	struct IndexBuffer;
}

/*
 */
struct Mesh
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec4 tangent;
		glm::vec3 binormal;
		glm::vec3 normal;
		glm::vec4 color;
		glm::vec2 uv;
	};

	uint32_t num_vertices {0};
	Vertex *vertices {nullptr};

	uint32_t num_indices {0};
	uint32_t *indices {nullptr};

	render::backend::VertexBuffer *vertex_buffer {nullptr};
	render::backend::IndexBuffer *index_buffer {nullptr};
};

template <>
struct TypeTraits<Mesh>
{
	static constexpr const char *name = "Mesh";
};

template <>
struct resources::ResourcePipeline<Mesh>
{
	static bool import(ResourceHandle<Mesh> handle, const resources::URI &uri, render::backend::Driver *driver);
	static bool importFromMemory(ResourceHandle<Mesh> handle, const uint8_t *data, size_t size, render::backend::Driver *driver);

	static void destroy(ResourceHandle<Mesh> handle, render::backend::Driver *driver);

	// TODO: move to render module API helpers
	static void createAssimp(ResourceHandle<Mesh> handle, render::backend::Driver *driver, const aiMesh *mesh);
	static void createCGLTF(ResourceHandle<Mesh> handle, render::backend::Driver *driver, const cgltf_mesh *mesh);
	static void createQuad(ResourceHandle<Mesh> handle, render::backend::Driver *driver, float size);
	static void createSkybox(ResourceHandle<Mesh> handle, render::backend::Driver *driver, float size);
};
