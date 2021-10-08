#pragma once

#include <game/World.h>
#include <render/backend/Driver.h>
#include <common/ResourceManager.h>

#include <glm/mat4x4.hpp>

class Mesh;
struct Texture;
class Shader;

namespace ecs::render
{
	// Resources
	struct EnvironmentTexture
	{
		resources::ResourceHandle<::Texture> baked_brdf;
		resources::ResourceHandle<::Texture> prefiltered_specular_cubemap;
		resources::ResourceHandle<::Texture> diffuse_irradiance_cubemap;
		::render::backend::BindSet *bindings {nullptr};
	};

	struct RenderMaterialData
	{
		resources::ResourceHandle<::Texture> albedo;
		resources::ResourceHandle<::Texture> normal;
		resources::ResourceHandle<::Texture> roughness;
		resources::ResourceHandle<::Texture> metalness;
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
	extern void init(game::World *world);
	extern void shutdown(game::World *world);

	// Systems
	extern void drawRenderables(
		const game::World *world,
		::render::backend::Driver *driver,
		uint8_t material_binding,
		::render::backend::PipelineState *pipeline_state,
		::render::backend::CommandBuffer *command_buffer
	);

	extern void drawSkyLights(
		const game::World *world,
		::render::backend::Driver *driver,
		uint8_t light_binding,
		::render::backend::PipelineState *pipeline_state,
		::render::backend::CommandBuffer *command_buffer
	);
}

template<>
struct TypeTraits<ecs::render::Transform>
{
	static constexpr const char *name = "ecs::render::Transform";
};

template<>
struct TypeTraits<ecs::render::SkyLight>
{
	static constexpr const char *name = "ecs::render::SkyLight";
};

template<>
struct TypeTraits<ecs::render::Renderable>
{
	static constexpr const char *name = "ecs::render::Renderable";
};
