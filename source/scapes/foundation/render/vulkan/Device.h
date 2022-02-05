#pragma once

#include <scapes/foundation/render/Device.h>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace scapes::foundation::render::vulkan
{
	class Context;
	class DescriptorSetLayoutCache;
	class ImageViewCache;
	class PipelineLayoutCache;
	class PipelineCache;

	struct VertexBuffer
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

	struct IndexBuffer
	{
		BufferType type {BufferType::STATIC};
		VkBuffer buffer {VK_NULL_HANDLE};
		VmaAllocation memory {VK_NULL_HANDLE};
		VkIndexType index_type {VK_INDEX_TYPE_UINT16};
		uint32_t num_indices {0};
	};

	struct Texture
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
		ImageViewCache *image_view_cache {nullptr};
	};

	struct FrameBuffer
	{
		enum
		{
			MAX_ATTACHMENTS = 16,
		};

		mutable VkFramebuffer frame_buffer {VK_NULL_HANDLE};
		VkExtent2D sizes {0, 0};

		uint32_t num_attachments {0};
		uint32_t num_layers {1};
		VkImageView attachment_views[MAX_ATTACHMENTS];
		VkFormat attachment_formats[MAX_ATTACHMENTS];
		VkSampleCountFlagBits attachment_samples[MAX_ATTACHMENTS];
	};

	struct RenderPass
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

	// TODO: move to sanity check
	static_assert(sizeof(VkClearValue) == sizeof(RenderPassClearValue));

	struct CommandBuffer
	{
		VkCommandBuffer command_buffer {VK_NULL_HANDLE};
		VkCommandBufferLevel level {VK_COMMAND_BUFFER_LEVEL_PRIMARY};
		VkSemaphore rendering_finished_gpu {VK_NULL_HANDLE};
		VkFence rendering_finished_cpu {VK_NULL_HANDLE};

		VkRenderPass render_pass {VK_NULL_HANDLE};
		VkSampleCountFlagBits max_samples {VK_SAMPLE_COUNT_1_BIT};
		uint32_t num_color_attachments {0};
	};

	struct UniformBuffer
	{
		BufferType type {BufferType::STATIC};
		VkBuffer buffer {VK_NULL_HANDLE};
		VmaAllocation memory {VK_NULL_HANDLE};
		uint32_t size {0};
		// TODO: static / dynamic fields
	};

	struct Shader
	{
		ShaderType type {ShaderType::FRAGMENT};
		VkShaderModule module {VK_NULL_HANDLE};
	};

	struct BindSet
	{
		enum
		{
			MAX_BINDINGS = 32,
		};

		union Data
		{
			struct Texture
			{
				VkImageLayout layout;
				VkImageView view;
				VkSampler sampler;
			} texture;
			struct UBO
			{
				VkBuffer buffer;
				uint32_t offset;
				uint32_t size;
			} ubo;
			struct TLAS
			{
				VkAccelerationStructureKHR acceleration_structure;
			} tlas;
		};

		VkDescriptorSetLayout set_layout {VK_NULL_HANDLE};
		VkDescriptorSet set {VK_NULL_HANDLE};

		VkDescriptorSetLayoutBinding bindings[MAX_BINDINGS];
		Data binding_data[MAX_BINDINGS];
		bool binding_used[MAX_BINDINGS];
		bool binding_dirty[MAX_BINDINGS];
	};

	struct GraphicsPipeline
	{
		enum
		{
			MAX_BIND_SETS = 16,
			MAX_VERTEX_STREAMS = 16,
			MAX_PUSH_CONSTANT_SIZE = 128, // TODO: use HW device capabilities for upper limit
			MAX_SHADERS = static_cast<int>(ShaderType::MAX),
		};

		// render state
		uint8_t depth_test : 1;
		uint8_t depth_write : 1;
		uint8_t blending : 1;

		VkCompareOp depth_compare_func {VK_COMPARE_OP_LESS_OR_EQUAL};
		VkCullModeFlags cull_mode {VK_CULL_MODE_BACK_BIT};
		VkPrimitiveTopology primitive_topology {VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};

		VkBlendFactor blend_src_factor {VK_BLEND_FACTOR_SRC_ALPHA};
		VkBlendFactor blend_dst_factor {VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA};

		VkViewport viewport;
		VkRect2D scissor;

		// resources
		uint8_t push_constants[MAX_PUSH_CONSTANT_SIZE];
		uint8_t push_constants_size {0};

		BindSet *bind_sets[MAX_BIND_SETS]; // TODO: make this safer
		uint8_t num_bind_sets {0};

		VertexBuffer *vertex_streams[MAX_VERTEX_STREAMS]; // TODO: make this safer
		uint8_t num_vertex_streams {0};

		VkShaderModule shaders[MAX_SHADERS];

		VkRenderPass render_pass {VK_NULL_HANDLE};
		VkSampleCountFlagBits max_samples {VK_SAMPLE_COUNT_1_BIT};
		uint8_t num_color_attachments {0};

		// internal mutable state
		VkPipeline pipeline {VK_NULL_HANDLE};
		VkPipelineLayout pipeline_layout {VK_NULL_HANDLE};

		// TODO: pipeline caches here
		// IDEA: get rid of pipeline layout cache, recreate layout if needed and be happy
	};
	
	struct AccelerationStructure
	{
		enum
		{
			MAX_GEOMETRIES = 8,
		};

		VkBuffer buffer {VK_NULL_HANDLE};
		VmaAllocation memory {VK_NULL_HANDLE};
		VkAccelerationStructureKHR acceleration_structure {VK_NULL_HANDLE};
		VkDeviceAddress device_address {0};
		VkDeviceSize size {0};
		VkDeviceSize update_scratch_size {0};
		VkDeviceSize build_scratch_size {0};
	};

	struct RayTracePipeline
	{
		enum
		{
			MAX_BIND_SETS = 16,
			MAX_RAYGEN_SHADERS = 32,
			MAX_HITGROUP_SHADERS = 32,
			MAX_MISS_SHADERS = 32,
		};

		// resources
		BindSet *bind_sets[MAX_BIND_SETS]; // TODO: make this safer
		uint8_t num_bind_sets {0};

		// shaders
		VkShaderModule raygen_shaders[MAX_RAYGEN_SHADERS];
		uint8_t num_raygen_shaders {0};

		VkShaderModule intersection_shaders[MAX_HITGROUP_SHADERS];
		VkShaderModule anyhit_shaders[MAX_HITGROUP_SHADERS];
		VkShaderModule closesthit_shaders[MAX_HITGROUP_SHADERS];
		uint8_t num_hitgroup_shaders {0};

		VkShaderModule miss_shaders[MAX_MISS_SHADERS];
		uint8_t num_miss_shaders {0};

		// internal mutable state
		VkPipeline pipeline {VK_NULL_HANDLE};
		VkPipelineLayout pipeline_layout {VK_NULL_HANDLE};

		VmaAllocation sbt_memory {VK_NULL_HANDLE};
		VkBuffer sbt_buffer {VK_NULL_HANDLE};

		VkDeviceAddress sbt_raygen_begin {0};
		VkDeviceAddress sbt_miss_begin {0};
		VkDeviceAddress sbt_hitgroup_begin {0};

		// TODO: pipeline caches here
		// IDEA: get rid of pipeline layout cache, recreate layout if needed and be happy
	};

	struct SwapChain
	{
		enum
		{
			MAX_IMAGES = 8,
		};

		VkSwapchainKHR swap_chain {VK_NULL_HANDLE};
		VkExtent2D sizes {0, 0};

		VkSurfaceKHR surface {VK_NULL_HANDLE};
		VkSurfaceCapabilitiesKHR surface_capabilities;
		VkSurfaceFormatKHR surface_format;

		uint32_t present_queue_family {0xFFFF};
		VkQueue present_queue {VK_NULL_HANDLE};
		VkPresentModeKHR present_mode {VK_PRESENT_MODE_FIFO_KHR};

		uint32_t num_images {0};
		uint32_t current_image {0};
		uint32_t current_frame {0};

		VkRenderPass dummy_render_pass {VK_NULL_HANDLE};

		VkFramebuffer frame_buffers[MAX_IMAGES];
		VkSemaphore image_available_gpu[MAX_IMAGES];
		VkImage images[MAX_IMAGES];
		VkImageView image_views[MAX_IMAGES];
	};

	class Device : public render::Device
	{
	public:
		Device(const char *application_name, const char *engine_name);
		virtual ~Device();

		SCAPES_INLINE const Context *getContext() const { return context; }

	public:
		render::VertexBuffer createVertexBuffer(
			BufferType type,
			uint16_t vertex_size,
			uint32_t num_vertices,
			uint8_t num_attributes,
			const render::VertexAttribute *attributes,
			const void *data
		) final;

		render::IndexBuffer createIndexBuffer(
			BufferType type,
			IndexFormat index_format,
			uint32_t num_indices,
			const void *data
		) final;

		render::Texture createTexture2D(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		render::Texture createTexture2DMultisample(
			uint32_t width,
			uint32_t height,
			Format format,
			Multisample samples
		) final;

		render::Texture createTexture2DArray(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			uint32_t num_layers,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1,
			uint32_t num_data_layers = 1
		) final;

		render::Texture createTexture3D(
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		render::Texture createTextureCube(
			uint32_t size,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		render::StorageImage createStorageImage(
			uint32_t width,
			uint32_t height,
			Format format
		) final;

		render::FrameBuffer createFrameBuffer(
			uint32_t num_attachments,
			const FrameBufferAttachment *attachments
		) final;

		render::RenderPass createRenderPass(
			uint32_t num_attachments,
			const RenderPassAttachment *attachments,
			const RenderPassDescription &description
		) final;

		render::RenderPass createRenderPass(
			render::SwapChain swap_chain,
			RenderPassLoadOp load_op,
			RenderPassStoreOp store_op,
			const RenderPassClearColor &clear_color
		) final;

		render::CommandBuffer createCommandBuffer(
			CommandBufferType type
		) final;

		render::UniformBuffer createUniformBuffer(
			BufferType type,
			uint32_t size,
			const void *data = nullptr
		) final;

		render::Shader createShaderFromSource(
			ShaderType type,
			uint32_t size,
			const char *data,
			const char *path = nullptr
		) final;

		render::Shader createShaderFromIL(
			ShaderType type,
			ShaderILType il_type,
			size_t size,
			const void *data
		) final;

		render::BindSet createBindSet(
		) final;

		render::GraphicsPipeline createGraphicsPipeline(
		) final;

		render::BottomLevelAccelerationStructure createBottomLevelAccelerationStructure(
			uint32_t num_geometries,
			const AccelerationStructureGeometry *geometries
		) final;

		render::TopLevelAccelerationStructure createTopLevelAccelerationStructure(
			uint32_t num_instances,
			const AccelerationStructureInstance *instances
		) final;

		render::RayTracePipeline createRayTracePipeline(
			// ???
		) final;

		render::SwapChain createSwapChain(
			void *native_window
		) final;

		void destroyVertexBuffer(render::VertexBuffer vertex_buffer) final;
		void destroyIndexBuffer(render::IndexBuffer index_buffer) final;
		void destroyTexture(render::Texture texture) final;
		void destroyStorageImage(render::StorageImage image) final;
		void destroyFrameBuffer(render::FrameBuffer frame_buffer) final;
		void destroyRenderPass(render::RenderPass render_pass) final;
		void destroyCommandBuffer(render::CommandBuffer command_buffer) final;
		void destroyUniformBuffer(render::UniformBuffer uniform_buffer) final;
		void destroyShader(render::Shader shader) final;
		void destroyBindSet(render::BindSet bind_set) final;
		void destroyGraphicsPipeline(render::GraphicsPipeline pipeline) final;
		void destroyBottomLevelAccelerationStructure(render::BottomLevelAccelerationStructure acceleration_structure) final;
		void destroyTopLevelAccelerationStructure(render::TopLevelAccelerationStructure acceleration_structure) final;
		void destroyRayTracePipeline(render::RayTracePipeline pipeline) final;
		void destroySwapChain(render::SwapChain swap_chain) final;

	public:
		bool isFlipped() final { return false; }
		Multisample getMaxSampleCount() final;

		uint32_t getNumSwapChainImages(render::SwapChain swap_chain) final;

		void setTextureSamplerWrapMode(render::Texture texture, SamplerWrapMode mode) final;
		void setTextureSamplerDepthCompare(render::Texture texture, bool enabled, DepthCompareFunc func) final;
		void generateTexture2DMipmaps(render::Texture texture) final;

	public:
		void *map(render::VertexBuffer vertex_buffer) final;
		void unmap(render::VertexBuffer vertex_buffer) final;

		void *map(render::IndexBuffer index_buffer) final;
		void unmap(render::IndexBuffer index_buffer) final;

		void *map(render::UniformBuffer uniform_buffer) final;
		void unmap(render::UniformBuffer uniform_buffer) final;

		void flush(render::BindSet bind_set) final;
		void flush(render::GraphicsPipeline pipeline) final;
		void flush(render::RayTracePipeline pipeline) final;

	public:
		bool acquire(
			render::SwapChain swap_chain,
			uint32_t *new_image
		) final;

		bool present(
			render::SwapChain swap_chain,
			uint32_t num_wait_command_buffers,
			const render::CommandBuffer *wait_command_buffers
		) final;

		void wait() final;
		bool wait(
			uint32_t num_wait_command_buffers,
			const render::CommandBuffer *wait_command_buffers
		) final;

	public:
		// bindings
		void bindUniformBuffer(
			render::BindSet bind_set,
			uint32_t binding,
			render::UniformBuffer uniform_buffer
		) final;

		void bindTexture(
			render::BindSet bind_set,
			uint32_t binding,
			render::Texture texture
		) final;

		void bindTexture(
			render::BindSet bind_set,
			uint32_t binding,
			render::StorageImage image
		) final;

		void bindTexture(
			render::BindSet bind_set,
			uint32_t binding,
			render::Texture texture,
			uint32_t base_mip,
			uint32_t num_mips,
			uint32_t base_layer,
			uint32_t num_layers
		) final;

		void bindTopLevelAccelerationStructure(
			render::BindSet bind_set,
			uint32_t binding,
			render::TopLevelAccelerationStructure tlas
		) final;

		void bindStorageImage(
			render::BindSet bind_set,
			uint32_t binding,
			render::StorageImage image
		) final;

	public:
		// raytrace pipeline state
		void clearBindSets(
			render::RayTracePipeline pipeline
		) final;

		void setBindSet(
			render::RayTracePipeline pipeline,
			uint8_t binding,
			render::BindSet bind_set
		) final;

		void clearRaygenShaders(
			render::RayTracePipeline pipeline
		) final;

		void addRaygenShader(
			render::RayTracePipeline pipeline,
			render::Shader shader
		) final;

		void clearHitGroupShaders(
			render::RayTracePipeline pipeline
		) final;

		void addHitGroupShader(
			render::RayTracePipeline pipeline,
			render::Shader intersection_shader,
			render::Shader anyhit_shader,
			render::Shader closesthit_shader
		) final;

		void clearMissShaders(
			render::RayTracePipeline pipeline
		) final;

		void addMissShader(
			render::RayTracePipeline pipeline,
			render::Shader shader
		) final;

	public:
		// pipeline state
		void clearPushConstants(
			render::GraphicsPipeline pipeline
		) final;

		void setPushConstants(
			render::GraphicsPipeline pipeline,
			uint8_t size,
			const void *data
		) final;

		void clearBindSets(
			render::GraphicsPipeline pipeline
		) final;

		void setBindSet(
			render::GraphicsPipeline pipeline,
			uint8_t binding,
			render::BindSet bind_set
		) final;

		void clearShaders(
			render::GraphicsPipeline pipeline
		) final;

		void setShader(
			render::GraphicsPipeline pipeline,
			ShaderType type,
			render::Shader shader
		) final;

		void clearVertexStreams(
			render::GraphicsPipeline pipeline
		) final;

		void setVertexStream(
			render::GraphicsPipeline pipeline,
			uint8_t binding,
			render::VertexBuffer vertex_buffer
		) final;

		void setViewport(
			render::GraphicsPipeline pipeline,
			int32_t x,
			int32_t y,
			uint32_t width,
			uint32_t height
		) final;

		void setScissor(
			render::GraphicsPipeline pipeline,
			int32_t x,
			int32_t y,
			uint32_t width,
			uint32_t height
		) final;

		void setPrimitiveType(
			render::GraphicsPipeline pipeline,
			RenderPrimitiveType type
		) final;

		void setCullMode(
			render::GraphicsPipeline pipeline,
			CullMode mode
		) final;

		void setDepthTest(
			render::GraphicsPipeline pipeline,
			bool enabled
		) final;

		void setDepthWrite(
			render::GraphicsPipeline pipeline,
			bool enabled
		) final;

		void setDepthCompareFunc(
			render::GraphicsPipeline pipeline,
			DepthCompareFunc func
		) final;

		void setBlending(
			render::GraphicsPipeline pipeline,
			bool enabled
		) final;

		void setBlendFactors(
			render::GraphicsPipeline pipeline,
			BlendFactor src_factor,
			BlendFactor dest_factor
		) final;

	public:
		// command buffers
		bool resetCommandBuffer(
			render::CommandBuffer command_buffer
		) final;

		bool beginCommandBuffer(
			render::CommandBuffer command_buffer
		) final;

		bool endCommandBuffer(
			render::CommandBuffer command_buffer
		) final;

		bool submit(
			render::CommandBuffer command_buffer
		) final;

		bool submitSyncked(
			render::CommandBuffer command_buffer,
			render::SwapChain wait_swap_chain
		) final;

		bool submitSyncked(
			render::CommandBuffer command_buffer,
			uint32_t num_wait_command_buffers,
			const render::CommandBuffer *wait_command_buffers
		) final;

	public:
		// render commands
		void beginRenderPass(
			render::CommandBuffer command_buffer,
			render::RenderPass render_pass,
			render::FrameBuffer frame_buffer
		) final;

		void beginRenderPass(
			render::CommandBuffer command_buffer,
			render::RenderPass render_pass,
			render::SwapChain swap_chain
		) final;

		void endRenderPass(
			render::CommandBuffer command_buffer
		) final;

		void drawIndexedPrimitiveInstanced(
			render::CommandBuffer command_buffer,
			render::GraphicsPipeline pipeline,
			render::IndexBuffer index_buffer,
			uint32_t num_indices,
			uint32_t base_index,
			int32_t base_vertex,
			uint32_t num_instances,
			uint32_t base_instance
		) final;

		void traceRays(
			render::CommandBuffer command_buffer,
			render::RayTracePipeline pipeline,
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			uint32_t raygen_shader_index
		) final;

	private:
		enum
		{
			WAIT_NANOSECONDS = 1000000000ul,
		};

		Context *context {nullptr};
		DescriptorSetLayoutCache *descriptor_set_layout_cache {nullptr};
		PipelineLayoutCache *pipeline_layout_cache {nullptr};
		PipelineCache *pipeline_cache {nullptr};
	};
}
