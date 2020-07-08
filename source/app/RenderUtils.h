#pragma once

#include <render/backend/driver.h>

namespace render
{
	class Texture;
	class Shader;
}

/*
 */
class RenderUtils
{
public:
	static render::Texture *createTexture2D(
		render::backend::Driver *driver,
		render::backend::Format format,
		uint32_t width,
		uint32_t height,
		uint32_t mips,
		const render::Shader *vertex_shader,
		const render::Shader *fragment_shader
	);

	static render::Texture *createTextureCube(
		render::backend::Driver *driver,
		render::backend::Format format,
		uint32_t size,
		uint32_t mips,
		const render::Shader *vertex_shader,
		const render::Shader *fragment_shader,
		const render::Texture *input
	);

	static render::Texture *hdriToCube(
		render::backend::Driver *driver,
		render::backend::Format format,
		uint32_t size,
		const render::Texture *hdri,
		const render::Shader *vertex_shader,
		const render::Shader *hrdi_fragment_shader,
		const render::Shader *prefilter_fragment_shader
	);
};
