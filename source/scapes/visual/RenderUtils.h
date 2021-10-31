#pragma once

#include <scapes/visual/Fwd.h>

namespace scapes::visual
{
	/*
	 */
	class RenderUtils
	{
	public:
		static void uploadToGPU(
			foundation::render::Device *device,
			MeshHandle mesh
		);

		static void RenderUtils::renderTexture2D(
			foundation::render::Device *device,
			TextureHandle result,
			MeshHandle fullscreen_quad,
			ShaderHandle vertex_shader,
			ShaderHandle fragment_shader
		);

		static void RenderUtils::renderTextureCube(
			foundation::render::Device *device,
			TextureHandle result,
			MeshHandle fullscreen_quad,
			ShaderHandle vertex_shader,
			ShaderHandle fragment_shader,
			TextureHandle input
		);

		static void renderHDRIToCube(
			foundation::render::Device *device,
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
