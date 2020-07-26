#include "RenderUtils.h"

#include "CubemapRenderer.h"
#include "Texture2DRenderer.h"

#include <render/Mesh.h>
#include <render/Texture.h>
#include <render/Shader.h>

using namespace render;

/*
 */
Texture *RenderUtils::createTexture2D(
	backend::Driver *driver,
	backend::Format format,
	uint32_t width,
	uint32_t height,
	uint32_t mips,
	const Shader *vertex_shader,
	const Shader *fragment_shader
)
{
	Texture *result = new Texture(driver);
	result->create2D(format, width, height, mips);

	Texture2DRenderer renderer(driver);
	renderer.init(result->getBackend());
	renderer.render(vertex_shader->getBackend(), fragment_shader->getBackend());

	return result;
}

Texture *RenderUtils::createTextureCube(
	backend::Driver *driver,
	backend::Format format,
	uint32_t size,
	uint32_t mips,
	const Shader *vertex_shader,
	const Shader *fragment_shader,
	const Texture *input
)
{
	Texture *result = new Texture(driver);
	result->createCube(backend::Format::R32G32B32A32_SFLOAT, size, size, mips);

	CubemapRenderer renderer(driver);
	renderer.init(result, 0);
	renderer.render(vertex_shader, fragment_shader, input);

	return result;
}

/*
 */
Texture *RenderUtils::hdriToCube(
	backend::Driver *driver,
	backend::Format format,
	uint32_t size,
	const Texture *hdri,
	const Shader *vertex_shader,
	const Shader *hdri_fragment_shader,
	const render::Shader *prefilter_fragment_shader
)
{
	uint32_t mips = static_cast<int>(std::floor(std::log2(size)) + 1);

	Texture *result = new Texture(driver);
	result->createCube(backend::Format::R32G32B32A32_SFLOAT, size, size, mips);

	CubemapRenderer renderer(driver);
	renderer.init(result, 0);
	renderer.render(vertex_shader, hdri_fragment_shader, hdri);

	for (uint32_t mip = 1; mip < mips; ++mip)
	{
		float roughness = static_cast<float>(mip) / mips;

		uint8_t size = static_cast<uint8_t>(sizeof(float));
		const uint8_t *data = reinterpret_cast<const uint8_t *>(&roughness);

		CubemapRenderer mip_renderer(driver);
		mip_renderer.init(result, mip);
		mip_renderer.render(vertex_shader, prefilter_fragment_shader, result, mip - 1, size, data);
	}

	return result;
}
