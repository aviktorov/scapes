#pragma once

#include <GLM/glm.hpp>

#include <map>
#include <string>
#include <vector>

class Light;

class Mesh;
class Texture;

namespace render::backend
{
	struct BindSet;
	struct UniformBuffer;
	class Driver;
}

struct aiNode;
struct aiScene;

/*
 */
class Scene
{
public:
	Scene(render::backend::Driver *driver)
		: driver(driver) { }

	~Scene();

	bool import(const char *path);
	void clear();

	inline size_t getNumNodes() const { return nodes.size(); }
	inline const Mesh *getNodeMesh(size_t index) const { return nodes[index].mesh; }
	inline const glm::mat4 &getNodeWorldTransform(size_t index) const { return nodes[index].transform; }
	inline render::backend::BindSet *getNodeBindings(size_t index) const
	{
		int32_t material_index = nodes[index].render_material_index;
		return materials[material_index].bindings;
	}

	inline void addLight(Light *light) { lights.push_back(light); }
	inline size_t getNumLights() const { return lights.size(); }
	inline const Light *getLight(size_t index) const { return lights[index]; }

private:
	struct RenderNode
	{
		const Mesh *mesh {nullptr};
		int32_t render_material_index {-1};
		glm::mat4 transform;
	};

	struct RenderMaterial
	{
		const Texture *albedo {nullptr};
		const Texture *normal {nullptr};
		const Texture *roughness {nullptr};
		const Texture *metalness {nullptr};
		render::backend::UniformBuffer * parameters {nullptr};
		render::backend::BindSet *bindings {nullptr};
	};

private:
	void importNodes(const aiScene *scene, const aiNode *root, const glm::mat4 &transform);

private:
	render::backend::Driver *driver {nullptr};

	std::vector<Mesh *> meshes;
	std::map<std::string, Texture *> textures;
	std::vector<RenderMaterial> materials;
	std::vector<RenderNode> nodes;

	// does not own
	std::vector<Light *> lights;
};
