#include "Mesh.h"

#include <render/backend/Driver.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <cgltf.h>

#include <iostream>

/*
 */
static void uploadToGPU(Mesh *mesh, render::backend::Driver *driver)
{
	assert(driver);
	assert(mesh);
	assert(mesh->num_vertices);
	assert(mesh->num_indices);
	assert(mesh->vertices);
	assert(mesh->indices);

	static render::backend::VertexAttribute mesh_attributes[6] =
	{
		{ render::backend::Format::R32G32B32_SFLOAT, offsetof(Mesh::Vertex, position) },
		{ render::backend::Format::R32G32_SFLOAT, offsetof(Mesh::Vertex, uv) },
		{ render::backend::Format::R32G32B32A32_SFLOAT, offsetof(Mesh::Vertex, tangent) },
		{ render::backend::Format::R32G32B32_SFLOAT, offsetof(Mesh::Vertex, binormal) },
		{ render::backend::Format::R32G32B32_SFLOAT, offsetof(Mesh::Vertex, normal) },
		{ render::backend::Format::R32G32B32A32_SFLOAT, offsetof(Mesh::Vertex, color) },
	};

	mesh->vertex_buffer = driver->createVertexBuffer(
		render::backend::BufferType::STATIC,
		sizeof(Mesh::Vertex), mesh->num_vertices,
		6, mesh_attributes,
		mesh->vertices
	);

	mesh->index_buffer = driver->createIndexBuffer(
		render::backend::BufferType::STATIC,
		render::backend::IndexFormat::UINT32,
		mesh->num_indices,
		mesh->indices
	);
}

/*
 */
void resources::ResourcePipeline<Mesh>::destroy(ResourceHandle<Mesh> handle, render::backend::Driver *driver)
{
	Mesh *mesh = handle.get();
	driver->destroyVertexBuffer(mesh->vertex_buffer);
	driver->destroyIndexBuffer(mesh->index_buffer);

	// TODO: use subresource pools
	delete[] mesh->vertices;
	delete[] mesh->indices;

	*mesh = {};
}

bool resources::ResourcePipeline<Mesh>::import(ResourceHandle<Mesh> handle, const URI &uri, render::backend::Driver *driver)
{
	FILE *file = fopen(uri, "rb");
	if (!file)
	{
		std::cerr << "Mesh::import(): can't open \"" << uri << "\" file" << std::endl;
		return false;
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8_t *data = new uint8_t[size];
	fread(data, sizeof(uint8_t), size, file);
	fclose(file);

	bool result = importFromMemory(handle, data, size, driver);
	delete[] data;

	return result;
}

bool resources::ResourcePipeline<Mesh>::importFromMemory(ResourceHandle<Mesh> handle, const uint8_t *data, size_t size, render::backend::Driver *driver)
{
	Assimp::Importer importer;

	const aiScene *scene = importer.ReadFileFromMemory(data, size, aiProcessPreset_TargetRealtime_MaxQuality);

	if (!scene)
	{
		std::cerr << "Mesh::import(): " << importer.GetErrorString() << std::endl;
		return false;
	}

	if (!scene->HasMeshes())
	{
		std::cerr << "Mesh::import(): model has no meshes" << std::endl;
		return false;
	}

	createAssimp(handle, driver, scene->mMeshes[0]);
	return true;
}

/*
 */
void resources::ResourcePipeline<Mesh>::createAssimp(ResourceHandle<Mesh> handle, render::backend::Driver *driver, const aiMesh *mesh)
{
	assert(mesh != nullptr);

	Mesh *result = handle.get();

	result->num_vertices = mesh->mNumVertices;
	result->num_indices = mesh->mNumFaces * 3;

	// TODO: use subresource pools
	result->vertices = new Mesh::Vertex[result->num_vertices];
	result->indices = new uint32_t[result->num_indices];

	aiVector3D *meshVertices = mesh->mVertices;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		result->vertices[i].position = glm::vec3(meshVertices[i].x, meshVertices[i].y, meshVertices[i].z);

	aiVector3D *meshTangents = mesh->mTangents;
	if (meshTangents)
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			result->vertices[i].tangent = glm::vec4(meshTangents[i].x, meshTangents[i].y, meshTangents[i].z, 0.0f);
	else
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			result->vertices[i].tangent = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

	aiVector3D *meshBinormals = mesh->mBitangents;
	if (meshBinormals)
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			result->vertices[i].binormal = glm::vec3(meshBinormals[i].x, meshBinormals[i].y, meshBinormals[i].z);
	else
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			result->vertices[i].binormal = glm::vec3(0.0f, 0.0f, 0.0f);

	aiVector3D *meshNormals = mesh->mNormals;
	if (meshNormals)
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			result->vertices[i].normal = glm::vec3(meshNormals[i].x, meshNormals[i].y, meshNormals[i].z);
	else
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			result->vertices[i].normal = glm::vec3(0.0f, 0.0f, 0.0f);

	aiVector3D *meshUVs = mesh->mTextureCoords[0];
	if (meshUVs)
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			result->vertices[i].uv = glm::vec2(meshUVs[i].x, 1.0f - meshUVs[i].y);
	else
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			result->vertices[i].uv = glm::vec2(0.0f, 0.0f);

	aiColor4D *meshColors = mesh->mColors[0];
	if (meshColors)
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			result->vertices[i].color = glm::vec4(meshColors[i].r, meshColors[i].g, meshColors[i].b, meshColors[i].a);
	else
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			result->vertices[i].color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	aiFace *meshFaces = mesh->mFaces;
	unsigned int index = 0;
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		for (unsigned int faceIndex = 0; faceIndex < meshFaces[i].mNumIndices; faceIndex++)
			result->indices[index++] = meshFaces[i].mIndices[faceIndex];

	uploadToGPU(result, driver);
}

void resources::ResourcePipeline<Mesh>::createCGLTF(ResourceHandle<Mesh> handle, render::backend::Driver *driver, const cgltf_mesh *mesh)
{
	assert(mesh);
	assert(mesh->primitives_count == 1);

	const cgltf_primitive &primitive = mesh->primitives[0];

	const cgltf_accessor *cgltf_positions = nullptr;
	const cgltf_accessor *cgltf_tangets = nullptr;
	const cgltf_accessor *cgltf_normals = nullptr;
	const cgltf_accessor *cgltf_uv = nullptr;
	const cgltf_accessor *cgltf_colors = nullptr;
	const cgltf_accessor *cgltf_indices = primitive.indices;

	for (cgltf_size i = 0; i < primitive.attributes_count; ++i)
	{
		const cgltf_attribute &attribute = primitive.attributes[i];
		switch (attribute.type)
		{
			case cgltf_attribute_type_position: cgltf_positions = attribute.data; break;
			case cgltf_attribute_type_normal: cgltf_normals = attribute.data; break;
			case cgltf_attribute_type_tangent: cgltf_tangets = attribute.data; break;
			case cgltf_attribute_type_texcoord: cgltf_uv = attribute.data; break;
			case cgltf_attribute_type_color: cgltf_colors = attribute.data; break;
		}
	}

	assert(cgltf_indices && cgltf_positions && cgltf_normals);
	assert(cgltf_positions->count == cgltf_normals->count);

	Mesh *result = handle.get();

	result->num_vertices = static_cast<uint32_t>(cgltf_positions->count);
	result->num_indices = static_cast<uint32_t>(cgltf_indices->count);

	// TODO: use subresource pools
	result->vertices = new Mesh::Vertex[result->num_vertices];
	result->indices = new uint32_t[result->num_indices];

	memset(result->vertices, 0, sizeof(Mesh::Vertex) * result->num_vertices);
	memset(result->indices, 0, sizeof(uint32_t) * result->num_indices);

	for (cgltf_size i = 0; i < cgltf_positions->count; i++)
	{
		cgltf_bool success = cgltf_accessor_read_float(cgltf_positions, i, (cgltf_float*)&result->vertices[i].position, 3);
		assert(success);
	}

	if (cgltf_tangets)
	{
		assert(cgltf_positions->count == cgltf_tangets->count);
		for (cgltf_size i = 0; i < cgltf_positions->count; i++)
		{
			cgltf_bool success = cgltf_accessor_read_float(cgltf_tangets, i, (cgltf_float*)&result->vertices[i].tangent, 4);
			assert(success);
		}
	}

	for (cgltf_size i = 0; i < cgltf_positions->count; i++)
	{
		cgltf_bool success = cgltf_accessor_read_float(cgltf_normals, i, (cgltf_float*)&result->vertices[i].normal, 3);
		assert(success);
	}

	for (cgltf_size i = 0; i < cgltf_positions->count; i++)
		result->vertices[i].binormal = glm::cross(result->vertices[i].normal, glm::vec3(result->vertices[i].tangent));

	if (cgltf_uv)
	{
		assert(cgltf_positions->count == cgltf_uv->count);
		for (cgltf_size i = 0; i < cgltf_positions->count; i++)
		{
			cgltf_bool success = cgltf_accessor_read_float(cgltf_uv, i, (cgltf_float*)&result->vertices[i].uv, 2);
			assert(success);
		}
	}

	if (cgltf_colors)
	{
		assert(cgltf_positions->count == cgltf_colors->count);
		for (cgltf_size i = 0; i < cgltf_positions->count; i++)
		{
			cgltf_bool success = cgltf_accessor_read_float(cgltf_colors, i, (cgltf_float*)&result->vertices[i].color, 4);
			assert(success);
		}
	}

	for (cgltf_size i = 0; i < cgltf_indices->count; i++)
	{
		cgltf_bool success = cgltf_accessor_read_uint(cgltf_indices, i, &result->indices[i], 1);
		assert(success);
	}

	uploadToGPU(result, driver);
}

void resources::ResourcePipeline<Mesh>::createSkybox(ResourceHandle<Mesh> handle, render::backend::Driver *driver, float size)
{
	Mesh *mesh = handle.get();

	mesh->num_vertices = 8;
	mesh->num_indices = 36;

	// TODO: use subresource pools
	mesh->vertices = new Mesh::Vertex[mesh->num_vertices];
	mesh->indices = new uint32_t[mesh->num_indices];

	memset(mesh->vertices, 0, sizeof(Mesh::Vertex) * mesh->num_vertices);

	float half_size = size * 0.5f;

	mesh->vertices[0].position = glm::vec3(-half_size, -half_size, -half_size);
	mesh->vertices[1].position = glm::vec3( half_size, -half_size, -half_size);
	mesh->vertices[2].position = glm::vec3( half_size,  half_size, -half_size);
	mesh->vertices[3].position = glm::vec3(-half_size,  half_size, -half_size);
	mesh->vertices[4].position = glm::vec3(-half_size, -half_size,  half_size);
	mesh->vertices[5].position = glm::vec3( half_size, -half_size,  half_size);
	mesh->vertices[6].position = glm::vec3( half_size,  half_size,  half_size);
	mesh->vertices[7].position = glm::vec3(-half_size,  half_size,  half_size);

	static uint32_t skybox_indices[] =
	{
		0, 1, 2, 2, 3, 0,
		1, 5, 6, 6, 2, 1,
		3, 2, 6, 6, 7, 3,
		5, 4, 6, 4, 7, 6,
		1, 0, 4, 4, 5, 1,
		4, 0, 3, 3, 7, 4,
	};

	memcpy(mesh->indices, skybox_indices, sizeof(uint32_t) * mesh->num_indices);

	uploadToGPU(mesh, driver);
}

void resources::ResourcePipeline<Mesh>::createQuad(ResourceHandle<Mesh> handle, render::backend::Driver *driver, float size)
{
	Mesh *mesh = handle.get();

	mesh->num_vertices = 4;
	mesh->num_indices = 6;

	// TODO: use subresource pools
	mesh->vertices = new Mesh::Vertex[mesh->num_vertices];
	mesh->indices = new uint32_t[mesh->num_indices];

	memset(mesh->vertices, 0, sizeof(Mesh::Vertex) * mesh->num_vertices);

	float half_size = size * 0.5f;

	mesh->vertices[0].position = glm::vec3(-half_size, -half_size, 0.0f);
	mesh->vertices[1].position = glm::vec3( half_size, -half_size, 0.0f);
	mesh->vertices[2].position = glm::vec3( half_size,  half_size, 0.0f);
	mesh->vertices[3].position = glm::vec3(-half_size,  half_size, 0.0f);

	mesh->vertices[0].uv = glm::vec2(0.0f, 0.0f);
	mesh->vertices[1].uv = glm::vec2(1.0f, 0.0f);
	mesh->vertices[2].uv = glm::vec2(1.0f, 1.0f);
	mesh->vertices[3].uv = glm::vec2(0.0f, 1.0f);

	static uint32_t quad_indices[] =
	{
		1, 0, 2, 3, 2, 0,
	};

	memcpy(mesh->indices, quad_indices, sizeof(uint32_t) * mesh->num_indices);

	uploadToGPU(mesh, driver);
}
