#pragma once

#include <scapes/visual/Fwd.h>

namespace scapes::visual
{
	/*
	 */
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
		foundation::render::PipelineState *pipeline_state {nullptr};
		foundation::render::CommandBuffer *command_buffer {nullptr};
		foundation::render::RenderPass *render_pass {nullptr};
		foundation::render::FrameBuffer *frame_buffer {nullptr};
	};
}
