#pragma once

#include <scapes/visual/Fwd.h>
#include <render/backend/Driver.h>

namespace scapes::visual
{
	/*
	 */
	class RenderUtils
	{
	public:
		static void uploadToGPU(
			::render::backend::Driver *driver,
			MeshHandle mesh
		);

		static void RenderUtils::renderTexture2D(
			::render::backend::Driver *driver,
			TextureHandle result,
			MeshHandle fullscreen_quad,
			ShaderHandle vertex_shader,
			ShaderHandle fragment_shader
		);

		static void RenderUtils::renderTextureCube(
			::render::backend::Driver *driver,
			TextureHandle result,
			MeshHandle fullscreen_quad,
			ShaderHandle vertex_shader,
			ShaderHandle fragment_shader,
			TextureHandle input
		);

		static void renderHDRIToCube(
			::render::backend::Driver *driver,
			TextureHandle result,
			MeshHandle fullscreen_quad,
			ShaderHandle vertex_shader,
			ShaderHandle hrdi_fragment_shader,
			ShaderHandle prefilter_fragment_shader,
			TextureHandle hdri,
			TextureHandle temp_cubemap
		);
	};
}
