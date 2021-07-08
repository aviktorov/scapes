#pragma once

#include <render/backend/Driver.h>

class Mesh;
class Texture;
class Shader;

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
	};

	struct Light
	{
		const ::Mesh *mesh {nullptr};
		const ::Shader *vertex_shader {nullptr};
		const ::Shader *fragment_shader {nullptr};
	};

	struct Renderable
	{
		const ::Mesh *mesh {nullptr};
		const RenderMaterialData *materials {nullptr};
	};

	// Systems
	// TODO:
}
