#include "RenderUtils.h"

#include "CubemapRenderer.h"
#include "Texture2DRenderer.h"

#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"

/*
 */
resources::ResourceHandle<Texture> RenderUtils::createTexture2D(
	resources::ResourceManager *resource_manager,
	render::backend::Driver *driver,
	render::backend::Format format,
	uint32_t width,
	uint32_t height,
	uint32_t mips,
	resources::ResourceHandle<Mesh> fullscreen_quad,
	const Shader *vertex_shader,
	const Shader *fragment_shader
)
{
	resources::ResourceHandle<Texture> result = resource_manager->create<Texture>();
	resources::ResourcePipeline<Texture>::create2D(result, driver, format, width, height, mips);

	Texture2DRenderer renderer(driver);
	renderer.init(result.get());
	renderer.render(fullscreen_quad.get(), vertex_shader, fragment_shader);

	return result;
}

resources::ResourceHandle<Texture> RenderUtils::createTextureCube(
	resources::ResourceManager *resource_manager,
	render::backend::Driver *driver,
	render::backend::Format format,
	uint32_t size,
	uint32_t mips,
	resources::ResourceHandle<Mesh> fullscreen_quad,
	const Shader *vertex_shader,
	const Shader *fragment_shader,
	resources::ResourceHandle<Texture> input
)
{
	resources::ResourceHandle<Texture> result = resource_manager->create<Texture>();
	resources::ResourcePipeline<Texture>::createCube(result, driver, format, size, mips);

	CubemapRenderer renderer(driver);
	renderer.init(result.get(), 0);
	renderer.render(fullscreen_quad.get(), vertex_shader, fragment_shader, input.get());

	return result;
}

/*
 */
resources::ResourceHandle<Texture> RenderUtils::hdriToCube(
	resources::ResourceManager *resource_manager,
	render::backend::Driver *driver,
	render::backend::Format format,
	uint32_t size,
	resources::ResourceHandle<Texture> hdri,
	resources::ResourceHandle<Mesh> fullscreen_quad,
	const Shader *vertex_shader,
	const Shader *hdri_fragment_shader,
	const Shader *prefilter_fragment_shader
)
{
	uint32_t mips = static_cast<int>(std::floor(std::log2(size)) + 1);

	resources::ResourceHandle<Texture> temp = resource_manager->create<Texture>();
	resources::ResourcePipeline<Texture>::createCube(temp, driver, format, size, 1);

	resources::ResourceHandle<Texture> result = resource_manager->create<Texture>();
	resources::ResourcePipeline<Texture>::createCube(result, driver, format, size, mips);

	CubemapRenderer renderer(driver);
	renderer.init(temp.get(), 0);
	renderer.render(fullscreen_quad.get(), vertex_shader, hdri_fragment_shader, hdri.get());

	for (uint32_t mip = 0; mip < mips; ++mip)
	{
		float roughness = static_cast<float>(mip) / mips;

		uint8_t size = static_cast<uint8_t>(sizeof(float));
		const uint8_t *data = reinterpret_cast<const uint8_t *>(&roughness);

		CubemapRenderer mip_renderer(driver);
		mip_renderer.init(result.get(), mip);
		mip_renderer.render(fullscreen_quad.get(), vertex_shader, prefilter_fragment_shader, temp.get(), size, data);
	}

	resource_manager->destroy(temp, driver);

	return result;
}
