#pragma once

#include <unordered_map>
#include <render/backend/Driver.h>

namespace render::shaders
{
	class Compiler;
}

class Mesh;
class Shader;
class Texture;

/*
 */
class ResourceManager
{
public:
	ResourceManager(render::backend::Driver *driver, render::shaders::Compiler *compiler)
		: driver(driver), compiler(compiler) {}

	~ResourceManager();

	void clear();

	Mesh *getMesh(int id) const;
	Mesh *createCubeMesh(int id, float size);
	Mesh *loadMesh(int id, const char *path);
	void unloadMesh(int id);

	Shader *getShader(int id) const;
	Shader *loadShader(int id, render::backend::ShaderType type, const char *path);
	bool reloadShader(int id);
	void unloadShader(int id);

	Texture *getTexture(int id) const;
	Texture *loadTexture(int id, const char *path);
	void unloadTexture(int id);

private:
	render::backend::Driver *driver {nullptr};
	render::shaders::Compiler *compiler {nullptr};

	std::unordered_map<int, Mesh *> meshes;
	std::unordered_map<int, Shader *> shaders;
	std::unordered_map<int, Texture *> textures;
};
