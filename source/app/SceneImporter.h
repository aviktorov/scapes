#pragma once

#include <map>
#include <vector>

class ApplicationResources;
class Mesh;
class Texture;

namespace ecs::render
{
	struct RenderMaterialData;
	struct EnvironmentTexture;
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

	bool importCGLTF(const char *path, const ApplicationResources *resources);
	bool importAssimp(const char *path, const ApplicationResources *resources);
	void clear();

	// TODO: move to resource manager
	const ecs::render::EnvironmentTexture *fetchEnvironmentTexture(
		const Texture *baked_brdf,
		const Texture *prefiltered_specular_cubemap,
		const Texture *diffuse_irradiance_cubemap
	);

private:
	render::backend::Driver *driver {nullptr};
	game::World *world {nullptr};

	// TODO: move to resource manager
	std::vector<Mesh *> meshes;
	std::vector<Texture *> textures;
	std::vector<ecs::render::RenderMaterialData *> materials;
	std::vector<ecs::render::EnvironmentTexture *> environment_textures;
};
