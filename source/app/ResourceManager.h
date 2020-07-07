#pragma once

#include <unordered_map>
#include <render/backend/Driver.h>

namespace render
{
	class Mesh;
	class Shader;
	class Texture;
}

/*
 */
class ResourceManager
{
public:
	ResourceManager(render::backend::Driver *driver)
		: driver(driver) {}

	render::Mesh *getMesh(int id) const;
	render::Mesh *createCubeMesh(int id, float size);
	render::Mesh *loadMesh(int id, const char *path);
	void unloadMesh(int id);

	render::Shader *getShader(int id) const;
	render::Shader *loadShader(int id, render::backend::ShaderType type, const char *path);
	bool reloadShader(int id);
	void unloadShader(int id);

	render::Texture *getTexture(int id) const;
	render::Texture *loadTexture(int id, const char *path);
	void unloadTexture(int id);

private:
	render::backend::Driver *driver {nullptr};

	std::unordered_map<int, render::Mesh *> meshes;
	std::unordered_map<int, render::Shader *> shaders;
	std::unordered_map<int, render::Texture *> textures;
};
