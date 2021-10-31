#include <scapes/visual/Resources.h>

#include <iostream>
#include <cassert>

using namespace scapes;
using namespace scapes::visual;

/*
 */
void ResourcePipeline<resources::Mesh>::destroy(
	foundation::resources::ResourceManager *resource_manager,
	MeshHandle handle,
	foundation::render::Device *device
)
{
	resources::Mesh *mesh = handle.get();

	device->destroyVertexBuffer(mesh->vertex_buffer);
	device->destroyIndexBuffer(mesh->index_buffer);

	// TODO: use subresource pools
	delete[] mesh->vertices;
	delete[] mesh->indices;

	*mesh = {};
}
