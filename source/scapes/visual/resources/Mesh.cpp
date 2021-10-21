#include <scapes/visual/Resources.h>

#include <cgltf.h>

#include <iostream>
#include <cassert>

using namespace scapes::visual;

/*
 */
void ResourcePipeline<resources::Mesh>::destroy(ResourceManager *resource_manager, MeshHandle handle, render::backend::Driver *driver)
{
	resources::Mesh *mesh = handle.get();

	driver->destroyVertexBuffer(mesh->vertex_buffer);
	driver->destroyIndexBuffer(mesh->index_buffer);

	// TODO: use subresource pools
	delete[] mesh->vertices;
	delete[] mesh->indices;

	*mesh = {};
}
