#pragma once

#include <render/backend/driver.h>

#include <GLM/glm.hpp>

#include <vector>

/*
 */
class VulkanMesh
{
public:
	VulkanMesh(render::backend::Driver *driver)
		: driver(driver) { }

	~VulkanMesh();

	inline uint32_t getNumIndices() const { return static_cast<uint32_t>(indices.size()); }
	inline render::backend::RenderPrimitive *getRenderPrimitive() const { return render_primitive; }

	bool loadFromFile(const char *path);

	void createSkybox(float size);
	void createQuad(float size);

	void uploadToGPU();
	void clearGPUData();
	void clearCPUData();

private:
	void createVertexBuffer();
	void createIndexBuffer();

private:
	render::backend::Driver *driver {nullptr};

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 tangent;
		glm::vec3 binormal;
		glm::vec3 normal;
		glm::vec3 color;
		glm::vec2 uv;
	};

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	render::backend::VertexBuffer *vertex_buffer {nullptr};
	render::backend::IndexBuffer *index_buffer {nullptr};
	render::backend::RenderPrimitive *render_primitive {nullptr};
};
