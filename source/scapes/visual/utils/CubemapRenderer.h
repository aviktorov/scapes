#pragma once

#include <scapes/Common.h>
#include <scapes/visual/Fwd.h>

namespace scapes::visual::utils
{
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
		foundation::render::BindSet bind_set {SCAPES_NULL_HANDLE};
		foundation::render::CommandBuffer command_buffer {SCAPES_NULL_HANDLE};
		foundation::render::FrameBuffer frame_buffer {SCAPES_NULL_HANDLE};
		foundation::render::RenderPass render_pass {SCAPES_NULL_HANDLE};
		foundation::render::UniformBuffer uniform_buffer {SCAPES_NULL_HANDLE};
		foundation::render::GraphicsPipeline graphics_pipeline {SCAPES_NULL_HANDLE};
	};
}
