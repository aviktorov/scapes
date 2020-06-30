#pragma once

#include "VulkanMesh.h"

#include <render/backend/driver.h>

class VulkanShader;
class VulkanTexture;

/*
 */
class VulkanTexture2DRenderer
{
public:
	VulkanTexture2DRenderer(render::backend::Driver *driver);

	void init(const VulkanTexture *target_texture);
	void shutdown();
	void render(const VulkanShader *vertex_shader, const VulkanShader *fragment_shader);

private:
	render::backend::Driver *driver {nullptr};
	render::backend::CommandBuffer *command_buffer {nullptr};
	render::backend::FrameBuffer *framebuffer {nullptr};

	VulkanMesh quad;
};
