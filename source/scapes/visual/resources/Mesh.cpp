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
