#pragma once

#include <map>
#include <vector>
#include <string>

#include <assimp/matrix4x4.h>

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

struct aiNode;
struct aiScene;

/*
 */
class Scene
{
public:
	Scene(render::backend::Driver *driver, game::World *world);
	~Scene();

	bool import(const char *path);
	void clear();

	// TODO: move to resource manager
	const ecs::render::EnvironmentTexture *fetchEnvironmentTexture(
		const Texture *baked_brdf,
		const Texture *prefiltered_specular_cubemap,
		const Texture *diffuse_irradiance_cubemap
	);

private:
	void importNodes(const aiScene *scene, const aiNode *root, const aiMatrix4x4 &transform);

private:
	render::backend::Driver *driver {nullptr};
	game::World *world {nullptr};

	// TODO: move to resource manager
	std::vector<Mesh *> meshes;
	std::map<std::string, Texture *> textures;
	std::vector<ecs::render::RenderMaterialData *> materials;
	std::vector<ecs::render::EnvironmentTexture *> environment_textures;
};
