#pragma once

#include <render/backend/driver.h>
#include <common/ResourceManager.h>

struct Mesh;
struct Texture;
class Shader;

/*
 */
class RenderUtils
{
public:
	static resources::ResourceHandle<Texture> createTexture2D(
		resources::ResourceManager *resource_manager,
		render::backend::Driver *driver,
		render::backend::Format format,
		uint32_t width,
		uint32_t height,
		uint32_t mips,
		resources::ResourceHandle<Mesh> fullscreen_quad,
		const Shader *vertex_shader,
		const Shader *fragment_shader
	);

	static resources::ResourceHandle<Texture> createTextureCube(
		resources::ResourceManager *resource_manager,
		render::backend::Driver *driver,
		render::backend::Format format,
		uint32_t size,
		uint32_t mips,
		resources::ResourceHandle<Mesh> fullscreen_quad,
		const Shader *vertex_shader,
		const Shader *fragment_shader,
		resources::ResourceHandle<Texture> input
	);

	static resources::ResourceHandle<Texture> hdriToCube(
		resources::ResourceManager *resource_manager,
		render::backend::Driver *driver,
		render::backend::Format format,
		uint32_t size,
		resources::ResourceHandle<Texture> hdri,
		resources::ResourceHandle<Mesh> fullscreen_quad,
		const Shader *vertex_shader,
		const Shader *hrdi_fragment_shader,
		const Shader *prefilter_fragment_shader
	);
};
