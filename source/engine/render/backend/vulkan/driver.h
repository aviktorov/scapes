#pragma once

#include "render/backend/driver.h"
#include <volk.h>

class VulkanContext;

namespace render::backend
{
	namespace vulkan
	{
		struct VertexBuffer : public render::backend::VertexBuffer
		{
			enum
			{
				MAX_ATTRIBUTES = 16,
			};

			VkBuffer buffer {VK_NULL_HANDLE};
			VkDeviceMemory memory {VK_NULL_HANDLE}; // TODO: replace by VMA later
			uint16_t vertex_size {0};
			uint32_t num_vertices {0};
			uint8_t num_attributes {0};
			VertexAttribute attributes[VertexBuffer::MAX_ATTRIBUTES];
		};

		struct IndexBuffer : public render::backend::IndexBuffer
		{
			VkBuffer buffer {VK_NULL_HANDLE};
			VkDeviceMemory memory {VK_NULL_HANDLE}; // TODO: replace by VMA later
			VkIndexType type {VK_INDEX_TYPE_UINT16};
			uint32_t num_indices {0};
			// TODO: static / dynamic fields
		};

		struct RenderPrimitive : public render::backend::RenderPrimitive
		{
			VkPrimitiveTopology topology {VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
			const VertexBuffer *vertex_buffer {nullptr};
			const IndexBuffer *index_buffer {nullptr};
		};

		struct Texture : public render::backend::Texture
		{
			VkImage image {VK_NULL_HANDLE};
			VkDeviceMemory memory {VK_NULL_HANDLE}; // TODO: replace by VMA later
			VkImageType type {VK_IMAGE_TYPE_2D};
			VkFormat format {VK_FORMAT_R8G8B8A8_UNORM};
			uint32_t width {0};
			uint32_t height {0};
			uint32_t depth {0};
			uint32_t num_layers {0};
			uint32_t num_mipmaps {0};
			VkImageTiling tiling {VK_IMAGE_TILING_OPTIMAL};
			VkSampleCountFlagBits samples {VK_SAMPLE_COUNT_1_BIT};
			VkImageCreateFlags flags {0};
		};

		struct FrameBuffer : public render::backend::FrameBuffer
		{
			enum
			{
				MAX_COLOR_ATTACHMENTS = 16,
			};

			VkFramebuffer framebuffer {VK_NULL_HANDLE};
			uint8_t num_color_attachments {0};
			VkImageView color_attachments[FrameBuffer::MAX_COLOR_ATTACHMENTS];
			VkImageView depthstencil_attachment {VK_NULL_HANDLE};
		};

		struct UniformBuffer : public render::backend::UniformBuffer
		{
			VkBuffer buffer {VK_NULL_HANDLE};
			VkDeviceMemory memory {VK_NULL_HANDLE}; // TODO: replace by VMA later
			uint32_t size {0};
			// TODO: static / dynamic fields
		};

		struct Shader : public render::backend::Shader
		{
			ShaderType type {ShaderType::Fragment};
			VkShaderModule shaderModule {VK_NULL_HANDLE};
		};
	}

	class VulkanDriver : public Driver
	{
	public:
		VertexBuffer *createVertexBuffer(
			BufferType type,
			uint16_t vertex_size,
			uint32_t num_vertices,
			uint8_t num_attributes,
			const VertexAttribute *attributes,
			const void *data
		) override;

		IndexBuffer *createIndexBuffer(
			BufferType type,
			IndexSize index_size,
			uint32_t num_indices,
			const void *data
		) override;

		RenderPrimitive *createRenderPrimitive(
			RenderPrimitiveType type,
			const VertexBuffer *vertex_buffer,
			const IndexBuffer *index_buffer
		) override;

		Texture *createTexture2D(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) override;

		Texture *createTexture2DArray(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			uint32_t num_layers,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1,
			uint32_t num_data_layers = 1
		) override;

		Texture *createTexture3D(
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) override;

		Texture *createTextureCube(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) override;

		FrameBuffer *createFrameBuffer(
			uint8_t num_color_attachments,
			const FrameBufferColorAttachment *color_attachments,
			const FrameBufferDepthStencilAttachment *depthstencil_attachment = nullptr
		) override;

		UniformBuffer *createUniformBuffer(
			BufferType type,
			uint32_t size,
			const void *data
		) override;

		Shader *createShaderFromSource(
			ShaderType type,
			uint32_t length,
			const char *source
		) override;

		Shader *createShaderFromBytecode(
			ShaderType type,
			uint32_t size,
			const void *data
		) override;

		void destroyVertexBuffer(VertexBuffer *vertex_buffer) override;
		void destroyIndexBuffer(IndexBuffer *index_buffer) override;
		void destroyRenderPrimitive(RenderPrimitive *render_primitive) override;
		void destroyTexture(Texture *texture) override;
		void destroyFrameBuffer(FrameBuffer *frame_buffer) override;
		void destroyUniformBuffer(UniformBuffer *uniform_buffer) override;
		void destroyShader(Shader *shader) override;

	public:

		// sequence
		void beginRenderPass(
			const FrameBuffer *frame_buffer
		) override;

		void endRenderPass() override;

		// bind
		void bindUniformBuffer(
			uint32_t unit,
			const UniformBuffer *uniform_buffer
		) override;

		void bindTexture(
			uint32_t unit,
			const Texture *texture
		) override;

		void bindShader(
			const Shader *shader
		) override;

		// draw
		void drawIndexedPrimitive(
			const RenderPrimitive *render_primitive
		) override;

		void drawIndexedPrimitiveInstanced(
			const RenderPrimitive *primitive,
			const VertexBuffer *instance_buffer,
			uint32_t num_instances,
			uint32_t offset
		) override;

	private:
		VulkanContext *context {nullptr};
	};
}
