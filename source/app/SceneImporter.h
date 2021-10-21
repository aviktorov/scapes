#pragma once

#include <scapes/visual/Fwd.h>

namespace game
{
	class World;
}

namespace render::backend
{
	class Driver;
}

class ApplicationResources;

struct cgltf_mesh;
struct aiMesh;

/*
 */
class SceneImporter
{
public:
	SceneImporter(game::World *world, scapes::visual::API *visual_api);
	~SceneImporter();

	bool importCGLTF(const char *path, ApplicationResources *resources);
	bool importAssimp(const char *path, ApplicationResources *resources);
	void clear();

private:
	scapes::visual::MeshHandle import_cgltf_mesh(const cgltf_mesh *mesh);
	scapes::visual::MeshHandle import_assimp_mesh(const aiMesh *mesh);

private:
	game::World *world {nullptr};
	scapes::visual::API *visual_api {nullptr};
};
