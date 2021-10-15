#include "RenderModule.h"
#include "Mesh.h"
#include "Shader.h"

namespace ecs::render
{
	static game::Query<Transform, Renderable> *renderable_query {nullptr};
	static game::Query<SkyLight> *skylight_query {nullptr};

	void init(game::World *world)
	{
		renderable_query = new game::Query<Transform, Renderable>(world);
		skylight_query = new game::Query<SkyLight>(world);
	}

	void shutdown(game::World *world)
	{
		delete renderable_query;
		delete skylight_query;
	}

	void drawRenderables(
		const game::World *world,
		::render::backend::Driver *driver,
		uint8_t material_binding,
		::render::backend::PipelineState *pipeline_state,
		::render::backend::CommandBuffer *command_buffer
	)
	{
		renderable_query->begin();

		while (renderable_query->next())
		{
			uint32_t num_items = renderable_query->getNumComponents();
			Transform *transforms = renderable_query->getComponents<Transform>(0);
			Renderable *renderables = renderable_query->getComponents<Renderable>(1);

			for (uint32_t i = 0; i < num_items; ++i)
			{
				const Transform &transform = transforms[i];
				const Renderable &renderable = renderables[i];
				const glm::mat4 &node_transform = transform.transform;

				const Mesh *mesh = renderable.mesh.get();
				::render::backend::BindSet *material_bindings = renderable.materials->bindings;

				driver->clearVertexStreams(pipeline_state);
				driver->setVertexStream(pipeline_state, 0, mesh->vertex_buffer);

				driver->setBindSet(pipeline_state, material_binding, material_bindings);
				driver->setPushConstants(pipeline_state, static_cast<uint8_t>(sizeof(glm::mat4)), &node_transform);

				driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, mesh->index_buffer, mesh->num_indices);
			}
		}
	}

	void drawSkyLights(
		const game::World *world,
		::render::backend::Driver *driver,
		uint8_t light_binding,
		::render::backend::PipelineState *pipeline_state,
		::render::backend::CommandBuffer *command_buffer
	)
	{
		skylight_query->begin();

		while (skylight_query->next())
		{
			uint32_t num_items = skylight_query->getNumComponents();
			SkyLight *skylights = skylight_query->getComponents<SkyLight>(0);

			for (uint32_t i = 0; i < num_items; ++i)
			{
				const SkyLight &light = skylights[i];
				const Mesh *mesh = light.mesh.get();

				driver->clearShaders(pipeline_state);
				driver->setShader(pipeline_state, ::render::backend::ShaderType::VERTEX, light.vertex_shader->getBackend());
				driver->setShader(pipeline_state, ::render::backend::ShaderType::FRAGMENT, light.fragment_shader->getBackend());

				driver->setBindSet(pipeline_state, light_binding, light.environment->bindings);

				driver->clearVertexStreams(pipeline_state);
				driver->setVertexStream(pipeline_state, 0, mesh->vertex_buffer);

				driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, mesh->index_buffer, mesh->num_indices);
			}
		}
	}
}
