#pragma once

#include <render/backend/driver.h>

class Texture;
class Shader;

/*
 */
class RenderUtils
{
public:
	static Texture *createTexture2D(
		render::backend::Driver *driver,
		render::backend::Format format,
		uint32_t width,
		uint32_t height,
		uint32_t mips,
		const Shader *vertex_shader,
		const Shader *fragment_shader
	);

	static Texture *createTextureCube(
		render::backend::Driver *driver,
		render::backend::Format format,
		uint32_t size,
		uint32_t mips,
		const Shader *vertex_shader,
		const Shader *fragment_shader,
		const Texture *input
	);

	static Texture *hdriToCube(
		render::backend::Driver *driver,
		render::backend::Format format,
		uint32_t size,
		const Texture *hdri,
		const Shader *vertex_shader,
		const Shader *hrdi_fragment_shader,
		const Shader *prefilter_fragment_shader
	);
};
