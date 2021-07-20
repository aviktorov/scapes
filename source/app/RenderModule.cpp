#include "RenderModule.h"
#include "Mesh.h"
#include "Shader.h"

#include <flecs.h>

namespace ecs::render
{
	static flecs::filter renderable_iter;
	static flecs::filter skylight_iter;

	void init(flecs::world *world)
	{
		renderable_iter = flecs::filter(*world)
			.include<Renderable>()
			.include<Transform>()
			.include_kind(flecs::MatchAll);

		skylight_iter = flecs::filter(*world)
			.include<SkyLight>()
			.include_kind(flecs::MatchAll);
	}

	void shutdown()
	{
	}

	void drawRenderables(
		const flecs::world *world,
		::render::backend::Driver *driver,
		uint8_t material_binding,
		::render::backend::PipelineState *pipeline_state,
		::render::backend::CommandBuffer *command_buffer
	)
	{
		for (flecs::iter it : world->filter(renderable_iter))
		{
			flecs::column<Renderable> renderables = it.table_column<Renderable>();
			flecs::column<Transform> transforms = it.table_column<Transform>();

			for (size_t row : it)
			{
				const Transform &transform = transforms[row];
				const Renderable &renderable = renderables[row];
				const glm::mat4 &node_transform = transform.transform;

				const Mesh *mesh = renderable.mesh;
				::render::backend::BindSet *material_bindings = renderable.materials->bindings;

				driver->clearVertexStreams(pipeline_state);
				driver->setVertexStream(pipeline_state, 0, mesh->getVertexBuffer());

				driver->setBindSet(pipeline_state, material_binding, material_bindings);
				driver->setPushConstants(pipeline_state, static_cast<uint8_t>(sizeof(glm::mat4)), &node_transform);

				driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, mesh->getIndexBuffer(), mesh->getNumIndices());
			}
		}
	}

	void drawSkyLights(
		const flecs::world *world,
		::render::backend::Driver *driver,
		uint8_t light_binding,
		::render::backend::PipelineState *pipeline_state,
		::render::backend::CommandBuffer *command_buffer
	)
	{
		for (flecs::iter it : world->filter(skylight_iter))
		{
			flecs::column<SkyLight> skylights = it.table_column<SkyLight>();

			for (size_t row : it)
			{
				const SkyLight &light = skylights[row];

				driver->clearShaders(pipeline_state);
				driver->setShader(pipeline_state, ::render::backend::ShaderType::VERTEX, light.vertex_shader->getBackend());
				driver->setShader(pipeline_state, ::render::backend::ShaderType::FRAGMENT, light.fragment_shader->getBackend());

				driver->setBindSet(pipeline_state, light_binding, light.environment->bindings);

				driver->clearVertexStreams(pipeline_state);
				driver->setVertexStream(pipeline_state, 0, light.mesh->getVertexBuffer());

				driver->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, light.mesh->getIndexBuffer(), light.mesh->getNumIndices());
			}
		}
	}
}
