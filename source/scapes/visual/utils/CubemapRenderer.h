#pragma once

#include <scapes/Common.h>
#include <scapes/visual/Fwd.h>

namespace scapes::visual::utils
{
	class CubemapRenderer
	{
	public:
		CubemapRenderer(hardware::Device *device);
		~CubemapRenderer();

		void init(
			const Texture *target_texture,
			uint32_t target_mip
		);

		void shutdown();

		void render(
			const Mesh *mesh,
			const Shader *vertex_shader,
			const Shader *geometry_shader,
			const Shader *fragment_shader,
			const Texture *input_texture,
			uint8_t push_constants_size = 0,
			const uint8_t *push_constants_data = nullptr
		);

	private:
		hardware::Device *device {nullptr};
		hardware::BindSet bind_set {SCAPES_NULL_HANDLE};
		hardware::CommandBuffer command_buffer {SCAPES_NULL_HANDLE};
		hardware::FrameBuffer frame_buffer {SCAPES_NULL_HANDLE};
		hardware::RenderPass render_pass {SCAPES_NULL_HANDLE};
		hardware::UniformBuffer uniform_buffer {SCAPES_NULL_HANDLE};
		hardware::GraphicsPipeline graphics_pipeline {SCAPES_NULL_HANDLE};
	};
}
