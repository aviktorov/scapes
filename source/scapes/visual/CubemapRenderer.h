#pragma once

#include <render/backend/Driver.h>

namespace scapes::visual
{
	namespace resources
	{
		struct Shader;
		struct Mesh;
		struct Texture;
	}

	/*
	 */
	class CubemapRenderer
	{
	public:
		CubemapRenderer(::render::backend::Driver *driver);
		~CubemapRenderer();

		void init(
			const resources::Texture *target_texture,
			uint32_t target_mip
		);

		void shutdown();

		void render(
			const resources::Mesh *mesh,
			const resources::Shader *vertex_shader,
			const resources::Shader *fragment_shader,
			const resources::Texture *input_texture,
			uint8_t push_constants_size = 0,
			const uint8_t *push_constants_data = nullptr
		);

	private:
		::render::backend::Driver *driver {nullptr};
		::render::backend::BindSet *bind_set {nullptr};
		::render::backend::CommandBuffer *command_buffer {nullptr};
		::render::backend::FrameBuffer *frame_buffer {nullptr};
		::render::backend::RenderPass *render_pass {nullptr};
		::render::backend::UniformBuffer *uniform_buffer {nullptr};
		::render::backend::PipelineState *pipeline_state {nullptr};
	};
}