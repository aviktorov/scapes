#pragma once

#include <render/backend/driver.h>

#include <GLM/glm.hpp>

#include <map>
#include <string>
#include <vector>

class VulkanMesh;
class VulkanTexture;

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
	inline const VulkanMesh *getNodeMesh(size_t index) const { return nodes[index].mesh; }
	inline const glm::mat4 &getNodeWorldTransform(size_t index) const { return nodes[index].transform; }
	inline render::backend::BindSet *getNodeBindings(size_t index) const
	{
		int32_t material_index = nodes[index].render_material_index;
		return materials[material_index].bindings;
	}

private:
	struct RenderNode
	{
		const VulkanMesh *mesh {nullptr};
		int32_t render_material_index {-1};
		glm::mat4 transform;
	};

	struct RenderMaterial
	{
		const VulkanTexture *albedo {nullptr};
		const VulkanTexture *normal {nullptr};
		const VulkanTexture *roughness {nullptr};
		const VulkanTexture *metalness {nullptr};
		render::backend::UniformBuffer * parameters {nullptr};
		render::backend::BindSet *bindings {nullptr};
	};

private:
	void importNodes(const aiScene *scene, const aiNode *root, const glm::mat4 &transform);

private:
	render::backend::Driver *driver {nullptr};

	std::vector<VulkanMesh *> meshes;
	std::map<std::string, VulkanTexture *> textures;
	std::vector<RenderMaterial> materials;
	std::vector<RenderNode> nodes;
};
