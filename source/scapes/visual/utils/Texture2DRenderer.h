#pragma once

#include <scapes/Common.h>
#include <scapes/visual/Fwd.h>

namespace scapes::visual::utils
{
	class Texture2DRenderer
	{
	public:
		Texture2DRenderer(foundation::render::Device *device);
		~Texture2DRenderer();

		void init(const resources::Texture *target_texture);
		void shutdown();

		void render(
			const resources::Mesh *mesh,
			const resources::Shader *vertex_shader,
			const resources::Shader *fragment_shader
		);

	private:
		foundation::render::Device *device {nullptr};
		foundation::render::GraphicsPipeline graphics_pipeline {SCAPES_NULL_HANDLE};
		foundation::render::CommandBuffer command_buffer {SCAPES_NULL_HANDLE};
		foundation::render::RenderPass render_pass {SCAPES_NULL_HANDLE};
		foundation::render::FrameBuffer frame_buffer {SCAPES_NULL_HANDLE};
	};
}
