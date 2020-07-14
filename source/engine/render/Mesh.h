#pragma once

#include <vector>
#include <GLM/glm.hpp>

struct aiMesh;

namespace render
{

	namespace backend
	{
		class Driver;
		struct VertexBuffer;
		struct IndexBuffer;
		struct RenderPrimitive;
	}

	/*
	*/
	class Mesh
	{
	public:
		Mesh(backend::Driver *driver)
			: driver(driver) { }

		~Mesh();

		inline uint32_t getNumIndices() const { return static_cast<uint32_t>(indices.size()); }
		inline const backend::RenderPrimitive *getRenderPrimitive() const { return render_primitive; }

		bool import(const char *path);
		bool import(const aiMesh *mesh);

		void createSkybox(float size);
		void createQuad(float size);

		void uploadToGPU();
		void clearGPUData();
		void clearCPUData();

	private:
		void createVertexBuffer();
		void createIndexBuffer();

	private:
		backend::Driver *driver {nullptr};

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

		backend::RenderPrimitive *render_primitive {nullptr};
	};
}
