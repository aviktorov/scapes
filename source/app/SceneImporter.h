#pragma once

#include <scapes/visual/Fwd.h>

struct cgltf_mesh;
struct aiMesh;

/*
 */
class SceneImporter
{
public:
	SceneImporter(scapes::visual::API *visual_api);
	~SceneImporter();

	bool importCGLTF(const char *path);
	bool importAssimp(const char *path);
	void clear();

private:
	scapes::visual::MeshHandle import_cgltf_mesh(const cgltf_mesh *mesh);
	scapes::visual::MeshHandle import_assimp_mesh(const aiMesh *mesh);

private:
	scapes::visual::API *visual_api {nullptr};
};
