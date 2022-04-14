#pragma once

#include <scapes/Common.h>
#include <scapes/visual/Fwd.h>

namespace scapes::visual::utils
{
	class Texture2DRenderer
	{
	public:
		Texture2DRenderer(hardware::Device *device);
		~Texture2DRenderer();

		void init(const Texture *target_texture);
		void shutdown();

		void render(
			const Mesh *mesh,
			const Shader *vertex_shader,
			const Shader *fragment_shader
		);

	private:
		hardware::Device *device {nullptr};
		hardware::GraphicsPipeline graphics_pipeline {SCAPES_NULL_HANDLE};
		hardware::CommandBuffer command_buffer {SCAPES_NULL_HANDLE};
		hardware::RenderPass render_pass {SCAPES_NULL_HANDLE};
		hardware::FrameBuffer frame_buffer {SCAPES_NULL_HANDLE};
	};
}
