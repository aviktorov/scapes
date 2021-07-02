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
	}

	/*
	*/
	class Mesh
	{
	public:
		Mesh(backend::Driver *driver)
			: driver(driver) { }

		~Mesh();

		inline uint32_t getNumIndices() const { return num_indices; }
		inline backend::VertexBuffer *getVertexBuffer() const { return vertex_buffer; }
		inline backend::IndexBuffer *getIndexBuffer() const { return index_buffer; }

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

		backend::VertexBuffer *vertex_buffer {nullptr};
		backend::IndexBuffer *index_buffer {nullptr};
		uint32_t num_indices {0};
	};
}
