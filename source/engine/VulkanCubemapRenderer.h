#pragma once

#include "VulkanMesh.h"

#include <render/backend/driver.h>

class VulkanShader;
class VulkanTexture;

/*
 */
class VulkanCubemapRenderer
{
public:
	VulkanCubemapRenderer(render::backend::Driver *driver);

	void init(
		const VulkanTexture *target_texture,
		uint32_t target_mip
	);

	void shutdown();

	void render(
		const VulkanShader *vertex_shader,
		const VulkanShader *fragment_shader,
		const VulkanTexture *input_texture,
		int input_mip = -1,
		uint8_t push_constants_size = 0,
		const uint8_t *push_constants_data = nullptr
	);

private:
	render::backend::Driver *driver {nullptr};
	render::backend::BindSet *bind_set {nullptr};
	render::backend::CommandBuffer *command_buffer {nullptr};
	render::backend::FrameBuffer *framebuffer {nullptr};
	render::backend::UniformBuffer *uniform_buffer {nullptr};

	VulkanMesh quad;
};
