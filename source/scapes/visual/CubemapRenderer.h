#pragma once

#include <scapes/visual/Fwd.h>

namespace scapes::visual
{
	/*
	 */
	class CubemapRenderer
	{
	public:
		CubemapRenderer(foundation::render::Device *device);
		~CubemapRenderer();

		void init(
			const resources::Texture *target_texture,
			uint32_t target_mip
		);

		void shutdown();

		void render(
			const resources::Mesh *mesh,
			const resources::Shader *vertex_shader,
			const resources::Shader *geometry_shader,
			const resources::Shader *fragment_shader,
			const resources::Texture *input_texture,
			uint8_t push_constants_size = 0,
			const uint8_t *push_constants_data = nullptr
		);

	private:
		foundation::render::Device *device {nullptr};
		foundation::render::BindSet *bind_set {nullptr};
		foundation::render::CommandBuffer *command_buffer {nullptr};
		foundation::render::FrameBuffer *frame_buffer {nullptr};
		foundation::render::RenderPass *render_pass {nullptr};
		foundation::render::UniformBuffer *uniform_buffer {nullptr};
		foundation::render::PipelineState *pipeline_state {nullptr};
	};
}