#pragma once

#include <game/World.h>
#include <render/backend/Driver.h>
#include <common/ResourceManager.h>

#include <glm/mat4x4.hpp>

struct Mesh;
struct Shader;
struct Texture;
struct IBLTexture;
struct RenderMaterial;

namespace ecs::render
{
	// Components
	struct Transform
	{
		glm::mat4 transform;
	};

	struct SkyLight
	{
		resources::ResourceHandle<::IBLTexture> ibl_environment;
		resources::ResourceHandle<::Mesh> mesh;
		resources::ResourceHandle<::Shader> vertex_shader;
		resources::ResourceHandle<::Shader> fragment_shader;
	};

	struct Renderable
	{
		resources::ResourceHandle<::Mesh> mesh;
		resources::ResourceHandle<::RenderMaterial> material;
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
