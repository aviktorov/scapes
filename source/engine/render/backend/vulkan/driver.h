#pragma once

#include "render/backend/driver.h"
#include <volk.h>
#include <vk_mem_alloc.h>

namespace render::backend::vulkan
{
	class Device;
	class Context;
	class DescriptorSetCache;
	class DescriptorSetLayoutCache;
	class ImageViewCache;
	class PipelineLayoutCache;
	class PipelineCache;
	class RenderPassCache;

	struct VertexBuffer : public render::backend::VertexBuffer
	{
		enum
		{
			MAX_ATTRIBUTES = 16,
		};

		BufferType type {BufferType::STATIC};
		VkBuffer buffer {VK_NULL_HANDLE};
		VmaAllocation memory {VK_NULL_HANDLE};
		uint16_t vertex_size {0};
		uint32_t num_vertices {0};
		uint8_t num_attributes {0};
		VkFormat attribute_formats[VertexBuffer::MAX_ATTRIBUTES];
		uint32_t attribute_offsets[VertexBuffer::MAX_ATTRIBUTES];
	};

	struct IndexBuffer : public render::backend::IndexBuffer
	{
		BufferType type {BufferType::STATIC};
		VkBuffer buffer {VK_NULL_HANDLE};
		VmaAllocation memory {VK_NULL_HANDLE};
		VkIndexType index_type {VK_INDEX_TYPE_UINT16};
		uint32_t num_indices {0};
	};

	struct Texture : public render::backend::Texture
	{
		VkImage image {VK_NULL_HANDLE};
		VkSampler sampler {VK_NULL_HANDLE};
		VmaAllocation memory {VK_NULL_HANDLE};
		VkImageType type {VK_IMAGE_TYPE_2D};
		VkFormat format {VK_FORMAT_UNDEFINED};
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
			MAX_ATTACHMENTS = 16,
		};

		VkFramebuffer framebuffer {VK_NULL_HANDLE};
		VkExtent2D sizes {0, 0};

		VkRenderPass dummy_render_pass {VK_NULL_HANDLE}; // TODO: move to render pass cache

		uint8_t num_attachments {0};
		VkImageView attachments[FrameBuffer::MAX_ATTACHMENTS];
		FrameBufferAttachmentType attachment_types[FrameBuffer::MAX_ATTACHMENTS];
		VkFormat attachment_formats[FrameBuffer::MAX_ATTACHMENTS];
		VkSampleCountFlagBits attachment_samples[FrameBuffer::MAX_ATTACHMENTS];
		bool attachment_resolve[FrameBuffer::MAX_ATTACHMENTS];
	};

	struct CommandBuffer : public render::backend::CommandBuffer
	{
		VkCommandBuffer command_buffer {VK_NULL_HANDLE};
		VkCommandBufferLevel level {VK_COMMAND_BUFFER_LEVEL_PRIMARY};
		VkSemaphore rendering_finished_gpu {VK_NULL_HANDLE};
		VkFence rendering_finished_cpu {VK_NULL_HANDLE};
	};

	struct UniformBuffer : public render::backend::UniformBuffer
	{
		BufferType type {BufferType::STATIC};
		VkBuffer buffer {VK_NULL_HANDLE};
		VmaAllocation memory {VK_NULL_HANDLE};
		uint32_t size {0};
		// TODO: static / dynamic fields
	};

	struct Shader : public render::backend::Shader
	{
		ShaderType type {ShaderType::FRAGMENT};
		VkShaderModule module {VK_NULL_HANDLE};
	};

	struct BindSet : public render::backend::BindSet
	{
		enum
		{
			MAX_BINDINGS = 32,
		};

		union Data
		{
			struct Texture
			{
				VkImageView view;
				VkSampler sampler;
			} texture;
			struct UBO
			{
				VkBuffer buffer;
				uint32_t offset;
				uint32_t size;
			} ubo;
		};

		VkDescriptorSetLayout set_layout {VK_NULL_HANDLE};
		VkDescriptorSet set {VK_NULL_HANDLE};

		VkDescriptorSetLayoutBinding bindings[MAX_BINDINGS];
		Data binding_data[MAX_BINDINGS];
		bool binding_used[MAX_BINDINGS];
		bool binding_dirty[MAX_BINDINGS];
	};

	struct SwapChain : public render::backend::SwapChain
	{
		enum
		{
			MAX_IMAGES = 8,
		};

		VkSwapchainKHR swap_chain {nullptr};
		VkExtent2D sizes {0, 0};

		VkSurfaceKHR surface {nullptr};
		VkSurfaceCapabilitiesKHR surface_capabilities;
		VkSurfaceFormatKHR surface_format;

		uint32_t present_queue_family {0xFFFF};
		VkQueue present_queue {VK_NULL_HANDLE};
		VkPresentModeKHR present_mode {VK_PRESENT_MODE_FIFO_KHR};

		VkImage msaa_color_image {VK_NULL_HANDLE};
		VkImageView msaa_color_view {VK_NULL_HANDLE};
		VmaAllocation msaa_color_memory {VK_NULL_HANDLE};

		VkImage depth_image {VK_NULL_HANDLE};
		VkImageView depth_view {VK_NULL_HANDLE};
		VmaAllocation depth_memory {VK_NULL_HANDLE};

		uint32_t num_images {0};
		uint32_t current_image {0};

		VkSemaphore image_available_gpu[SwapChain::MAX_IMAGES];
		VkImage images[SwapChain::MAX_IMAGES];
	};

	class Driver : public backend::Driver
	{
	public:
		Driver(const char *application_name, const char *engine_name);
		virtual ~Driver();

		inline const Device *getDevice() const { return device; }

	public:
		backend::VertexBuffer *createVertexBuffer(
			BufferType type,
			uint16_t vertex_size,
			uint32_t num_vertices,
			uint8_t num_attributes,
			const backend::VertexAttribute *attributes,
			const void *data
		) override;

		backend::IndexBuffer *createIndexBuffer(
			BufferType type,
			IndexFormat index_format,
			uint32_t num_indices,
			const void *data
		) override;

		backend::Texture *createTexture2D(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			Format format,
			Multisample samples = Multisample::COUNT_1,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) override;

		backend::Texture *createTexture2DArray(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			uint32_t num_layers,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1,
			uint32_t num_data_layers = 1
		) override;

		backend::Texture *createTexture3D(
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) override;

		backend::Texture *createTextureCube(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) override;

		backend::FrameBuffer *createFrameBuffer(
			uint8_t num_attachments,
			const FrameBufferAttachment *attachments
		) override;

		backend::CommandBuffer *createCommandBuffer(
			CommandBufferType type
		) override;

		backend::UniformBuffer *createUniformBuffer(
			BufferType type,
			uint32_t size,
			const void *data = nullptr
		) override;

		backend::Shader *createShaderFromSource(
			ShaderType type,
			uint32_t size,
			const char *data,
			const char *path = nullptr
		) override;

		backend::Shader *createShaderFromBytecode(
			ShaderType type,
			uint32_t size,
			const void *data
		) override;

		backend::BindSet *createBindSet(
		) override;

		backend::SwapChain *createSwapChain(
			void *native_window,
			uint32_t width,
			uint32_t height
		) override;

		void destroyVertexBuffer(backend::VertexBuffer *vertex_buffer) override;
		void destroyIndexBuffer(backend::IndexBuffer *index_buffer) override;
		void destroyTexture(backend::Texture *texture) override;
		void destroyFrameBuffer(backend::FrameBuffer *frame_buffer) override;
		void destroyCommandBuffer(backend::CommandBuffer *command_buffer) override;
		void destroyUniformBuffer(backend::UniformBuffer *uniform_buffer) override;
		void destroyShader(backend::Shader *shader) override;
		void destroyBindSet(backend::BindSet *bind_set) override;
		void destroySwapChain(backend::SwapChain *swap_chain) override;

	public:
		Multisample getMaxSampleCount() override;
		Format getOptimalDepthFormat() override;

		Format getSwapChainImageFormat(const backend::SwapChain *swap_chain) override;
		uint32_t getNumSwapChainImages(const backend::SwapChain *swap_chain) override;

		void setTextureSamplerWrapMode(backend::Texture *texture, SamplerWrapMode mode) override;

	public:
		void generateTexture2DMipmaps(backend::Texture *texture) override;

		void *map(backend::VertexBuffer *vertex_buffer) override;
		void unmap(backend::VertexBuffer *vertex_buffer) override;

		void *map(backend::IndexBuffer *index_buffer) override;
		void unmap(backend::IndexBuffer *index_buffer) override;

		void *map(backend::UniformBuffer *uniform_buffer) override;
		void unmap(backend::UniformBuffer *uniform_buffer) override;

		void wait() override;
		bool wait(
			uint32_t num_wait_command_buffers,
			backend::CommandBuffer * const *wait_command_buffers
		) override;

		bool acquire(
			backend::SwapChain *swap_chain,
			uint32_t *new_image
		) override;

		bool present(
			backend::SwapChain *swap_chain,
			uint32_t num_wait_command_buffers,
			backend::CommandBuffer * const *wait_command_buffers
		) override;

	public:
		// bindings
		void bindUniformBuffer(
			backend::BindSet *bind_set,
			uint32_t binding,
			const backend::UniformBuffer *uniform_buffer
		) override;

		void bindTexture(
			backend::BindSet *bind_set,
			uint32_t binding,
			const backend::Texture *texture
		) override;

		void bindTexture(
			backend::BindSet *bind_set,
			uint32_t binding,
			const backend::Texture *texture,
			uint32_t base_mip,
			uint32_t num_mips,
			uint32_t base_layer,
			uint32_t num_layers
		) override;

	public:
		// pipeline state
		void clearPushConstants(
		) override;

		void setPushConstants(
			uint8_t size,
			const void *data
		) override;

		void clearBindSets(
		) override;

		void allocateBindSets(
			uint8_t num_bind_sets
		) override;

		void pushBindSet(
			backend::BindSet *bind_set
		) override;

		void setBindSet(
			uint32_t binding,
			backend::BindSet *bind_set
		) override;

		void clearShaders(
		) override;

		void setShader(
			ShaderType type,
			const backend::Shader *shader
		) override;

	public:
		// render state
		void setViewport(
			float x,
			float y,
			float width,
			float height
		) override;

		void setScissor(
			int32_t x,
			int32_t y,
			uint32_t width,
			uint32_t height
		) override;

		void setCullMode(
			CullMode mode
		) override;

		void setDepthTest(
			bool enabled
		) override;

		void setDepthWrite(
			bool enabled
		) override;

		void setDepthCompareFunc(
			DepthCompareFunc func
		) override;

		void setBlending(
			bool enabled
		) override;

		void setBlendFactors(
			BlendFactor src_factor,
			BlendFactor dest_factor
		) override;

	public:
		// command buffers
		bool resetCommandBuffer(
			backend::CommandBuffer *command_buffer
		) override;

		bool beginCommandBuffer(
			backend::CommandBuffer *command_buffer
		) override;

		bool endCommandBuffer(
			backend::CommandBuffer *command_buffer
		) override;

		bool submit(
			backend::CommandBuffer *command_buffer
		) override;

		bool submitSyncked(
			backend::CommandBuffer *command_buffer,
			const backend::SwapChain *wait_swap_chain
		) override;

		bool submitSyncked(
			backend::CommandBuffer *command_buffer,
			uint32_t num_wait_command_buffers,
			backend::CommandBuffer * const *wait_command_buffers
		) override;

	public:
		// render commands
		void beginRenderPass(
			backend::CommandBuffer *command_buffer,
			const backend::FrameBuffer *frame_buffer,
			const RenderPassInfo *info
		) override;

		void endRenderPass(
			backend::CommandBuffer *command_buffer
		) override;

		void drawIndexedPrimitive(
			backend::CommandBuffer *command_buffer,
			const backend::RenderPrimitive *render_primitive
		) override;

	private:
		Device *device {nullptr};
		Context *context {nullptr};
		DescriptorSetLayoutCache *descriptor_set_layout_cache {nullptr};
		ImageViewCache *image_view_cache {nullptr};
		PipelineLayoutCache *pipeline_layout_cache {nullptr};
		PipelineCache *pipeline_cache {nullptr};
		RenderPassCache *render_pass_cache {nullptr};
	};
}
