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

		mutable VkFramebuffer frame_buffer {VK_NULL_HANDLE};
		VkExtent2D sizes {0, 0};

		uint32_t num_attachments {0};
		VkImageView attachment_views[MAX_ATTACHMENTS];
		VkFormat attachment_formats[MAX_ATTACHMENTS];
		VkSampleCountFlagBits attachment_samples[MAX_ATTACHMENTS];
	};

	struct RenderPass : public render::backend::RenderPass
	{
		enum
		{
			MAX_ATTACHMENTS = 16,
		};

		VkRenderPass render_pass {VK_NULL_HANDLE};

		uint32_t num_attachments {0};
		VkFormat attachment_formats[MAX_ATTACHMENTS];
		VkSampleCountFlagBits attachment_samples[MAX_ATTACHMENTS];
		VkAttachmentLoadOp attachment_load_ops[MAX_ATTACHMENTS];
		VkAttachmentStoreOp attachment_store_ops[MAX_ATTACHMENTS];
		VkClearValue attachment_clear_values[MAX_ATTACHMENTS];

		VkSampleCountFlagBits max_samples {VK_SAMPLE_COUNT_1_BIT};
		uint32_t num_color_attachments {0};
	};

	static_assert(sizeof(VkClearValue) == sizeof(RenderPassClearValue));

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

		uint32_t num_images {0};
		uint32_t current_image {0};

		VkRenderPass dummy_render_pass {VK_NULL_HANDLE};

		VkFramebuffer frame_buffers[MAX_IMAGES];
		VkSemaphore image_available_gpu[MAX_IMAGES];
		VkImage images[MAX_IMAGES];
		VkImageView image_views[MAX_IMAGES];
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
		) final;

		backend::IndexBuffer *createIndexBuffer(
			BufferType type,
			IndexFormat index_format,
			uint32_t num_indices,
			const void *data
		) final;

		backend::Texture *createTexture2D(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		backend::Texture *createTexture2DMultisample(
			uint32_t width,
			uint32_t height,
			Format format,
			Multisample samples
		) final;

		backend::Texture *createTexture2DArray(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			uint32_t num_layers,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1,
			uint32_t num_data_layers = 1
		) final;

		backend::Texture *createTexture3D(
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		backend::Texture *createTextureCube(
			uint32_t size,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		backend::FrameBuffer *createFrameBuffer(
			uint32_t num_attachments,
			const FrameBufferAttachment *attachments
		) final;

		backend::RenderPass *createRenderPass(
			uint32_t num_attachments,
			const RenderPassAttachment *attachments,
			const RenderPassDescription &description
		) final;

		backend::RenderPass *createRenderPass(
			const backend::SwapChain *swap_chain,
			RenderPassLoadOp load_op,
			RenderPassStoreOp store_op,
			const RenderPassClearColor *clear_color
		) final;

		backend::CommandBuffer *createCommandBuffer(
			CommandBufferType type
		) final;

		backend::UniformBuffer *createUniformBuffer(
			BufferType type,
			uint32_t size,
			const void *data = nullptr
		) final;

		backend::Shader *createShaderFromSource(
			ShaderType type,
			uint32_t size,
			const char *data,
			const char *path = nullptr
		) final;

		backend::Shader *createShaderFromIL(
			const shaders::ShaderIL *shader_il
		) final;

		backend::BindSet *createBindSet(
		) final;

		backend::SwapChain *createSwapChain(
			void *native_window
		) final;

		void destroyVertexBuffer(backend::VertexBuffer *vertex_buffer) final;
		void destroyIndexBuffer(backend::IndexBuffer *index_buffer) final;
		void destroyTexture(backend::Texture *texture) final;
		void destroyFrameBuffer(backend::FrameBuffer *frame_buffer) final;
		void destroyRenderPass(backend::RenderPass *render_pass) final;
		void destroyCommandBuffer(backend::CommandBuffer *command_buffer) final;
		void destroyUniformBuffer(backend::UniformBuffer *uniform_buffer) final;
		void destroyShader(backend::Shader *shader) final;
		void destroyBindSet(backend::BindSet *bind_set) final;
		void destroySwapChain(backend::SwapChain *swap_chain) final;

	public:
		bool isFlipped() final { return false; }
		Multisample getMaxSampleCount() final;

		uint32_t getNumSwapChainImages(const backend::SwapChain *swap_chain) final;

		void setTextureSamplerWrapMode(backend::Texture *texture, SamplerWrapMode mode) final;
		void setTextureSamplerDepthCompare(backend::Texture *texture, bool enabled, DepthCompareFunc func) final;
		void generateTexture2DMipmaps(backend::Texture *texture) final;

	public:
		void *map(backend::VertexBuffer *vertex_buffer) final;
		void unmap(backend::VertexBuffer *vertex_buffer) final;

		void *map(backend::IndexBuffer *index_buffer) final;
		void unmap(backend::IndexBuffer *index_buffer) final;

		void *map(backend::UniformBuffer *uniform_buffer) final;
		void unmap(backend::UniformBuffer *uniform_buffer) final;

	public:
		bool acquire(
			backend::SwapChain *swap_chain,
			uint32_t *new_image
		) final;

		bool present(
			backend::SwapChain *swap_chain,
			uint32_t num_wait_command_buffers,
			backend::CommandBuffer * const *wait_command_buffers
		) final;

		void wait() final;
		bool wait(
			uint32_t num_wait_command_buffers,
			backend::CommandBuffer * const *wait_command_buffers
		) final;

	public:
		// bindings
		void bindUniformBuffer(
			backend::BindSet *bind_set,
			uint32_t binding,
			const backend::UniformBuffer *uniform_buffer
		) final;

		void bindTexture(
			backend::BindSet *bind_set,
			uint32_t binding,
			const backend::Texture *texture
		) final;

		void bindTexture(
			backend::BindSet *bind_set,
			uint32_t binding,
			const backend::Texture *texture,
			uint32_t base_mip,
			uint32_t num_mips,
			uint32_t base_layer,
			uint32_t num_layers
		) final;

	public:
		// pipeline state
		void clearPushConstants(
		) final;

		void setPushConstants(
			uint8_t size,
			const void *data
		) final;

		void clearBindSets(
		) final;

		void allocateBindSets(
			uint8_t num_bind_sets
		) final;

		void pushBindSet(
			backend::BindSet *bind_set
		) final;

		void setBindSet(
			uint32_t binding,
			backend::BindSet *bind_set
		) final;

		void clearShaders(
		) final;

		void setShader(
			ShaderType type,
			const backend::Shader *shader
		) final;

	public:
		// render state
		void setViewport(
			float x,
			float y,
			float width,
			float height
		) final;

		void setScissor(
			int32_t x,
			int32_t y,
			uint32_t width,
			uint32_t height
		) final;

		void setCullMode(
			CullMode mode
		) final;

		void setDepthTest(
			bool enabled
		) final;

		void setDepthWrite(
			bool enabled
		) final;

		void setDepthCompareFunc(
			DepthCompareFunc func
		) final;

		void setBlending(
			bool enabled
		) final;

		void setBlendFactors(
			BlendFactor src_factor,
			BlendFactor dest_factor
		) final;

	public:
		// command buffers
		bool resetCommandBuffer(
			backend::CommandBuffer *command_buffer
		) final;

		bool beginCommandBuffer(
			backend::CommandBuffer *command_buffer
		) final;

		bool endCommandBuffer(
			backend::CommandBuffer *command_buffer
		) final;

		bool submit(
			backend::CommandBuffer *command_buffer
		) final;

		bool submitSyncked(
			backend::CommandBuffer *command_buffer,
			const backend::SwapChain *wait_swap_chain
		) final;

		bool submitSyncked(
			backend::CommandBuffer *command_buffer,
			uint32_t num_wait_command_buffers,
			backend::CommandBuffer * const *wait_command_buffers
		) final;

	public:
		// render commands
		void beginRenderPass(
			backend::CommandBuffer *command_buffer,
			const backend::RenderPass *render_pass,
			const backend::FrameBuffer *frame_buffer
		) final;

		void beginRenderPass(
			backend::CommandBuffer *command_buffer,
			const backend::RenderPass *render_pass,
			const backend::SwapChain *swap_chain
		) final;

		void endRenderPass(
			backend::CommandBuffer *command_buffer
		) final;

		void drawIndexedPrimitive(
			backend::CommandBuffer *command_buffer,
			const backend::RenderPrimitive *render_primitive
		) final;

	private:
		Device *device {nullptr};
		Context *context {nullptr};
		DescriptorSetLayoutCache *descriptor_set_layout_cache {nullptr};
		ImageViewCache *image_view_cache {nullptr};
		PipelineLayoutCache *pipeline_layout_cache {nullptr};
		PipelineCache *pipeline_cache {nullptr};
	};
}
