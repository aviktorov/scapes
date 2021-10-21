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
	class Texture2DRenderer
	{
	public:
		Texture2DRenderer(::render::backend::Driver *driver);
		~Texture2DRenderer();

		void init(const resources::Texture *target_texture);
		void shutdown();

		void render(
			const resources::Mesh *mesh,
			const resources::Shader *vertex_shader,
			const resources::Shader *fragment_shader
		);

	private:
		::render::backend::Driver *driver {nullptr};
		::render::backend::PipelineState *pipeline_state {nullptr};
		::render::backend::CommandBuffer *command_buffer {nullptr};
		::render::backend::RenderPass *render_pass {nullptr};
		::render::backend::FrameBuffer *frame_buffer {nullptr};
	};
}
