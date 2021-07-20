#pragma once

#include <render/backend/Driver.h>
#include <glm/mat4x4.hpp>

class Mesh;
class Texture;
class Shader;

namespace flecs
{
	class world;
}

namespace ecs::render
{
	// Resources
	struct EnvironmentTexture
	{
		const ::Texture *baked_brdf {nullptr};
		const ::Texture *prefiltered_specular_cubemap {nullptr};
		const ::Texture *diffuse_irradiance_cubemap {nullptr};
		::render::backend::BindSet *bindings {nullptr};
	};

	struct RenderMaterialData
	{
		const ::Texture *albedo {nullptr};
		const ::Texture *normal {nullptr};
		const ::Texture *roughness {nullptr};
		const ::Texture *metalness {nullptr};
		::render::backend::UniformBuffer *parameters {nullptr};
		::render::backend::BindSet *bindings {nullptr};
	};

	// Components
	struct Transform
	{
		glm::mat4 transform;
	};

	struct SkyLight
	{
		const EnvironmentTexture *environment {nullptr};
		const ::Mesh *mesh {nullptr};
		const ::Shader *vertex_shader {nullptr};
		const ::Shader *fragment_shader {nullptr};
	};

	struct Renderable
	{
		const ::Mesh *mesh {nullptr};
		const RenderMaterialData *materials {nullptr};
	};

	// TODO: upgrade this into class

	// Module
	extern void init(flecs::world *world);
	extern void shutdown();

	// Systems
	extern void drawRenderables(
		const flecs::world *world,
		::render::backend::Driver *driver,
		uint8_t material_binding,
		::render::backend::PipelineState *pipeline_state,
		::render::backend::CommandBuffer *command_buffer
	);

	extern void drawSkyLights(
		const flecs::world *world,
		::render::backend::Driver *driver,
		uint8_t light_binding,
		::render::backend::PipelineState *pipeline_state,
		::render::backend::CommandBuffer *command_buffer
	);
}
