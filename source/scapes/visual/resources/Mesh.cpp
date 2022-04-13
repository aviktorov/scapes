#include <scapes/visual/Resources.h>

#include <iostream>
#include <cassert>

using namespace scapes;
using namespace scapes::visual;

/*
 */
size_t ResourceTraits<resources::Mesh>::size()
{
	return sizeof(resources::Mesh);
}

void ResourceTraits<resources::Mesh>::create(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	foundation::render::Device *device
)
{
	resources::Mesh *mesh = reinterpret_cast<resources::Mesh *>(memory);

	*mesh = {};
	mesh->device = device;
}

void ResourceTraits<resources::Mesh>::create(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	foundation::render::Device *device,
	uint32_t num_vertices,
	resources::Mesh::Vertex *vertices,
	uint32_t num_indices,
	uint32_t *indices
)
{
	resources::Mesh *mesh = reinterpret_cast<resources::Mesh *>(memory);

	*mesh = {};
	mesh->device = device;
	mesh->num_vertices = num_vertices;
	mesh->num_indices = num_indices;

	// TODO: use subresource pools
	mesh->vertices = new resources::Mesh::Vertex[mesh->num_vertices];
	mesh->indices = new uint32_t[mesh->num_indices];

	memcpy(mesh->vertices, vertices, sizeof(resources::Mesh::Vertex) * mesh->num_vertices);
	memcpy(mesh->indices, indices, sizeof(uint32_t) * mesh->num_indices);

	flushToGPU(resource_manager, memory);
}

void ResourceTraits<resources::Mesh>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	resources::Mesh *mesh = reinterpret_cast<resources::Mesh *>(memory);
	foundation::render::Device *device = mesh->device;

	assert(device);

	device->destroyVertexBuffer(mesh->vertex_buffer);
	device->destroyIndexBuffer(mesh->index_buffer);

	// TODO: use subresource pools
	delete[] mesh->vertices;
	delete[] mesh->indices;

	*mesh = {};
}

foundation::resources::hash_t ResourceTraits<resources::Mesh>::fetchHash(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	return 0;
}

void ResourceTraits<resources::Mesh>::flushToGPU(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	resources::Mesh *mesh = reinterpret_cast<resources::Mesh *>(memory);
	foundation::render::Device *device = mesh->device;

	assert(device);
	assert(mesh->num_vertices);
	assert(mesh->num_indices);
	assert(mesh->vertices);
	assert(mesh->indices);

	static foundation::render::VertexAttribute mesh_attributes[6] =
	{
		{ foundation::render::Format::R32G32B32_SFLOAT, offsetof(resources::Mesh::Vertex, position) },
		{ foundation::render::Format::R32G32_SFLOAT, offsetof(resources::Mesh::Vertex, uv) },
		{ foundation::render::Format::R32G32B32A32_SFLOAT, offsetof(resources::Mesh::Vertex, tangent) },
		{ foundation::render::Format::R32G32B32_SFLOAT, offsetof(resources::Mesh::Vertex, binormal) },
		{ foundation::render::Format::R32G32B32_SFLOAT, offsetof(resources::Mesh::Vertex, normal) },
		{ foundation::render::Format::R32G32B32A32_SFLOAT, offsetof(resources::Mesh::Vertex, color) },
	};

	device->destroyVertexBuffer(mesh->vertex_buffer);
	mesh->vertex_buffer = device->createVertexBuffer(
		foundation::render::BufferType::STATIC,
		sizeof(resources::Mesh::Vertex), mesh->num_vertices,
		6, mesh_attributes,
		mesh->vertices
	);

	device->destroyIndexBuffer(mesh->index_buffer);
	mesh->index_buffer = device->createIndexBuffer(
		foundation::render::BufferType::STATIC,
		foundation::render::IndexFormat::UINT32,
		mesh->num_indices,
		mesh->indices
	);
}


bool ResourceTraits<resources::Mesh>::reload(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	return false;
}
