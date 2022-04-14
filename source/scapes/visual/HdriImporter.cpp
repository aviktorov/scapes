#include "HdriImporter.h"

#include "CubemapRenderer.h"
#include "Texture2DRenderer.h"

#include <scapes/visual/API.h>
#include <scapes/visual/Resources.h>
#include <scapes/visual/Components.h>

#include <scapes/foundation/components/Components.h>
#include <scapes/foundation/game/World.h>
#include <scapes/foundation/game/Entity.h>
#include <scapes/foundation/math/Math.h>

namespace scapes::visual
{
	/*
	 */
	HdriImporter *HdriImporter::create(const HdriImporter::CreateOptions &options)
	{
		return new impl::HdriImporter(options);
	}

	void HdriImporter::destroy(HdriImporter *importer)
	{
		delete importer;
	}
}

namespace scapes::visual::impl
{
	/*
	 */
	HdriImporter::HdriImporter(const HdriImporter::CreateOptions &options)
		: resource_manager(options.resource_manager)
		, world(options.world)
		, device(options.device)
		, compiler(options.compiler)
		, unit_quad(options.unit_quad)
		, unit_cube(options.unit_cube)
		, default_vertex(options.default_vertex)
		, default_cubemap_geometry(options.default_cubemap_geometry)
		, bake_brdf_fragment(options.bake_brdf_fragment)
		, equirectangular_projection_fragment(options.equirectangular_projection_fragment)
		, diffuse_irradiance_fragment(options.diffuse_irradiance_fragment)
		, prefiltered_specular_fragment(options.prefiltered_specular_fragment)
	{
		assert(resource_manager);
		assert(world);
		assert(device);
		assert(compiler);
	}

	HdriImporter::~HdriImporter()
	{

	}

	/*
	 */
	TextureHandle HdriImporter::bakeBRDF(foundation::render::Format format, uint32_t size)
	{
		TextureHandle result = resource_manager->create<resources::Texture>(device, format, size, size);
		result->gpu_data = device->createTexture2D(result->width, result->height, result->mip_levels, result->format);

		Texture2DRenderer renderer(device);
		renderer.init(result.get());
		renderer.render(unit_quad.get(), default_vertex.get(), bake_brdf_fragment.get());
		renderer.shutdown();

		device->setTextureSamplerWrapMode(result->gpu_data, foundation::render::SamplerWrapMode::CLAMP_TO_EDGE);

		return result;
	}

	IBLTextureHandle HdriImporter::import(const foundation::io::URI &uri, foundation::render::Format format, uint32_t size, TextureHandle baked_brdf)
	{
		TextureHandle hdri_texture = resource_manager->fetch<resources::Texture>(uri, device);
		if (hdri_texture.get() == nullptr)
			return IBLTextureHandle();

		uint32_t mips = static_cast<int>(std::floor(std::log2(size)) + 1);

		resources::Texture diffuse_irradiance = {};
		diffuse_irradiance.format = format;
		diffuse_irradiance.width = size;
		diffuse_irradiance.height = size;
		diffuse_irradiance.mip_levels = 1;
		diffuse_irradiance.layers = 6;
		diffuse_irradiance.gpu_data = device->createTextureCube(size, 1, format);

		resources::Texture temp_cubemap = {};
		temp_cubemap.format = format;
		temp_cubemap.width = size;
		temp_cubemap.height = size;
		temp_cubemap.mip_levels = 1;
		temp_cubemap.layers = 6;
		temp_cubemap.gpu_data = device->createTextureCube(size, 1, format);

		resources::Texture prefiltered_specular = {};
		prefiltered_specular.format = format;
		prefiltered_specular.width = size;
		prefiltered_specular.height = size;
		prefiltered_specular.mip_levels = mips;
		prefiltered_specular.layers = 6;
		prefiltered_specular.gpu_data = device->createTextureCube(size, mips, format);

		CubemapRenderer renderer(device);
		renderer.init(&temp_cubemap, 0);
		renderer.render(
			unit_cube.get(),
			default_vertex.get(),
			default_cubemap_geometry.get(),
			equirectangular_projection_fragment.get(),
			hdri_texture.get()
		);
		renderer.shutdown();

		resource_manager->destroy(hdri_texture);

		for (uint32_t mip = 0; mip < prefiltered_specular.mip_levels; ++mip)
		{
			float roughness = static_cast<float>(mip) / prefiltered_specular.mip_levels;

			uint8_t size = static_cast<uint8_t>(sizeof(float));
			const uint8_t *data = reinterpret_cast<const uint8_t *>(&roughness);

			renderer.init(&prefiltered_specular, mip);
			renderer.render(
				unit_cube.get(),
				default_vertex.get(),
				default_cubemap_geometry.get(),
				prefiltered_specular_fragment.get(),
				&temp_cubemap,
				size,
				data
			);
			renderer.shutdown();
		}

		renderer.init(&diffuse_irradiance, 0);
		renderer.render(
			unit_cube.get(),
			default_vertex.get(),
			default_cubemap_geometry.get(),
			diffuse_irradiance_fragment.get(),
			&temp_cubemap
		);
		renderer.shutdown();

		device->destroyTexture(temp_cubemap.gpu_data);

		IBLTextureHandle result = resource_manager->create<resources::IBLTexture>(device);

		result->diffuse_irradiance_cubemap = diffuse_irradiance.gpu_data;
		result->prefiltered_specular_cubemap = prefiltered_specular.gpu_data;

		result->baked_brdf = baked_brdf;

		result->bindings = device->createBindSet();
		device->bindTexture(result->bindings, 0, result->baked_brdf->gpu_data);
		device->bindTexture(result->bindings, 1, result->prefiltered_specular_cubemap);
		device->bindTexture(result->bindings, 2, result->diffuse_irradiance_cubemap);

		return result;
	}
}
