#pragma once

#include <common/ResourceManager.h>

#include <map>
#include <vector>

class ApplicationResources;
struct Mesh;
struct Texture;

namespace ecs::render
{
	struct RenderMaterial;
	struct IBLTexture;
}

namespace game
{
	class World;
}

namespace render::backend
{
	class Driver;
}

/*
 */
class SceneImporter
{
public:
	SceneImporter(render::backend::Driver *driver, game::World *world);
	~SceneImporter();

	bool importCGLTF(const char *path, ApplicationResources *resources);
	bool importAssimp(const char *path, ApplicationResources *resources);
	void clear();

private:
	render::backend::Driver *driver {nullptr};
	game::World *world {nullptr};
};
