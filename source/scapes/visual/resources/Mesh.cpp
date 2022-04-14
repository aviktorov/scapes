#include <scapes/visual/Mesh.h>
#include <scapes/visual/hardware/Device.h>

#include <iostream>
#include <cassert>

using namespace scapes;
using namespace scapes::visual;

/*
 */
size_t ResourceTraits<Mesh>::size()
{
	return sizeof(Mesh);
}

void ResourceTraits<Mesh>::create(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	scapes::visual::hardware::Device *device
)
{
	Mesh *mesh = reinterpret_cast<Mesh *>(memory);

	*mesh = {};
	mesh->device = device;
}

void ResourceTraits<Mesh>::create(
	foundation::resources::ResourceManager *resource_manager,
	void *memory,
	scapes::visual::hardware::Device *device,
	uint32_t num_vertices,
	Mesh::Vertex *vertices,
	uint32_t num_indices,
	uint32_t *indices
)
{
	Mesh *mesh = reinterpret_cast<Mesh *>(memory);

	*mesh = {};
	mesh->device = device;
	mesh->num_vertices = num_vertices;
	mesh->num_indices = num_indices;

	// TODO: use subresource pools
	mesh->vertices = new Mesh::Vertex[mesh->num_vertices];
	mesh->indices = new uint32_t[mesh->num_indices];

	memcpy(mesh->vertices, vertices, sizeof(Mesh::Vertex) * mesh->num_vertices);
	memcpy(mesh->indices, indices, sizeof(uint32_t) * mesh->num_indices);

	flushToGPU(resource_manager, memory);
}

void ResourceTraits<Mesh>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	Mesh *mesh = reinterpret_cast<Mesh *>(memory);
	scapes::visual::hardware::Device *device = mesh->device;

	assert(device);

	device->destroyVertexBuffer(mesh->vertex_buffer);
	device->destroyIndexBuffer(mesh->index_buffer);

	// TODO: use subresource pools
	delete[] mesh->vertices;
	delete[] mesh->indices;

	*mesh = {};
}

foundation::resources::hash_t ResourceTraits<Mesh>::fetchHash(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	return 0;
}

void ResourceTraits<Mesh>::flushToGPU(
	foundation::resources::ResourceManager *resource_manager,
	void *memory
)
{
	Mesh *mesh = reinterpret_cast<Mesh *>(memory);
	scapes::visual::hardware::Device *device = mesh->device;

	assert(device);
	assert(mesh->num_vertices);
	assert(mesh->num_indices);
	assert(mesh->vertices);
	assert(mesh->indices);

	static scapes::visual::hardware::VertexAttribute mesh_attributes[6] =
	{
		{ scapes::visual::hardware::Format::R32G32B32_SFLOAT, offsetof(Mesh::Vertex, position) },
		{ scapes::visual::hardware::Format::R32G32_SFLOAT, offsetof(Mesh::Vertex, uv) },
		{ scapes::visual::hardware::Format::R32G32B32A32_SFLOAT, offsetof(Mesh::Vertex, tangent) },
		{ scapes::visual::hardware::Format::R32G32B32_SFLOAT, offsetof(Mesh::Vertex, binormal) },
		{ scapes::visual::hardware::Format::R32G32B32_SFLOAT, offsetof(Mesh::Vertex, normal) },
		{ scapes::visual::hardware::Format::R32G32B32A32_SFLOAT, offsetof(Mesh::Vertex, color) },
	};

	device->destroyVertexBuffer(mesh->vertex_buffer);
	mesh->vertex_buffer = device->createVertexBuffer(
		scapes::visual::hardware::BufferType::STATIC,
		sizeof(Mesh::Vertex), mesh->num_vertices,
		6, mesh_attributes,
		mesh->vertices
	);

	device->destroyIndexBuffer(mesh->index_buffer);
	mesh->index_buffer = device->createIndexBuffer(
		scapes::visual::hardware::BufferType::STATIC,
		scapes::visual::hardware::IndexFormat::UINT32,
		mesh->num_indices,
		mesh->indices
	);
}


bool ResourceTraits<Mesh>::reload(
	foundation::resources::ResourceManager *resource_manager,
	foundation::io::FileSystem *file_system,
	void *memory,
	const foundation::io::URI &uri
)
{
	return false;
}
