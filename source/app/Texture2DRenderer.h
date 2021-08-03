#pragma once

#include "Mesh.h"
#include <render/backend/Driver.h>

class Shader;
class Texture;

/*
 */
class Texture2DRenderer
{
public:
	Texture2DRenderer(render::backend::Driver *driver);
	~Texture2DRenderer();

	void init(const Texture *target_texture);
	void shutdown();
	void render(const Shader *vertex_shader, const Shader *fragment_shader);

private:
	render::backend::Driver *driver {nullptr};
	render::backend::PipelineState *pipeline_state {nullptr};
	render::backend::CommandBuffer *command_buffer {nullptr};
	render::backend::RenderPass *render_pass {nullptr};
	render::backend::FrameBuffer *frame_buffer {nullptr};

	Mesh quad;
};
