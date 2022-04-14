#pragma once

#include <scapes/visual/hardware/Device.h>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace scapes::visual::hardware::vulkan
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
		VkImageLayout layout {VK_IMAGE_LAYOUT_UNDEFINED};
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

	class Device : public hardware::Device
	{
	public:
		Device(const char *application_name, const char *engine_name);
		virtual ~Device();

		SCAPES_INLINE const Context *getContext() const { return context; }

	public:
		hardware::VertexBuffer createVertexBuffer(
			BufferType type,
			uint16_t vertex_size,
			uint32_t num_vertices,
			uint8_t num_attributes,
			const hardware::VertexAttribute *attributes,
			const void *data
		) final;

		hardware::IndexBuffer createIndexBuffer(
			BufferType type,
			IndexFormat index_format,
			uint32_t num_indices,
			const void *data
		) final;

		hardware::Texture createTexture2D(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		hardware::Texture createTexture2DMultisample(
			uint32_t width,
			uint32_t height,
			Format format,
			Multisample samples
		) final;

		hardware::Texture createTexture2DArray(
			uint32_t width,
			uint32_t height,
			uint32_t num_mipmaps,
			uint32_t num_layers,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1,
			uint32_t num_data_layers = 1
		) final;

		hardware::Texture createTexture3D(
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		hardware::Texture createTextureCube(
			uint32_t size,
			uint32_t num_mipmaps,
			Format format,
			const void *data = nullptr,
			uint32_t num_data_mipmaps = 1
		) final;

		hardware::Texture createTextureStorage(
			uint32_t width,
			uint32_t height,
			Format format
		) final;

		hardware::FrameBuffer createFrameBuffer(
			uint32_t num_attachments,
			const FrameBufferAttachment *attachments
		) final;

		hardware::RenderPass createRenderPass(
			uint32_t num_attachments,
			const RenderPassAttachment *attachments,
			const RenderPassDescription &description
		) final;

		hardware::RenderPass createRenderPass(
			hardware::SwapChain swap_chain,
			RenderPassLoadOp load_op,
			RenderPassStoreOp store_op,
			const RenderPassClearColor &clear_color
		) final;

		hardware::CommandBuffer createCommandBuffer(
			CommandBufferType type
		) final;

		hardware::UniformBuffer createUniformBuffer(
			BufferType type,
			uint32_t size,
			const void *data = nullptr
		) final;

		hardware::Shader createShaderFromSource(
			ShaderType type,
			uint32_t size,
			const char *data,
			const char *path = nullptr
		) final;

		hardware::Shader createShaderFromIL(
			ShaderType type,
			ShaderILType il_type,
			size_t size,
			const void *data
		) final;

		hardware::BindSet createBindSet(
		) final;

		hardware::GraphicsPipeline createGraphicsPipeline(
		) final;

		hardware::BottomLevelAccelerationStructure createBottomLevelAccelerationStructure(
			uint32_t num_geometries,
			const AccelerationStructureGeometry *geometries
		) final;

		hardware::TopLevelAccelerationStructure createTopLevelAccelerationStructure(
			uint32_t num_instances,
			const AccelerationStructureInstance *instances
		) final;

		hardware::RayTracePipeline createRayTracePipeline(
			// ???
		) final;

		hardware::SwapChain createSwapChain(
			void *native_window
		) final;

		void destroyVertexBuffer(hardware::VertexBuffer vertex_buffer) final;
		void destroyIndexBuffer(hardware::IndexBuffer index_buffer) final;
		void destroyTexture(hardware::Texture texture) final;
		void destroyFrameBuffer(hardware::FrameBuffer frame_buffer) final;
		void destroyRenderPass(hardware::RenderPass render_pass) final;
		void destroyCommandBuffer(hardware::CommandBuffer command_buffer) final;
		void destroyUniformBuffer(hardware::UniformBuffer uniform_buffer) final;
		void destroyShader(hardware::Shader shader) final;
		void destroyBindSet(hardware::BindSet bind_set) final;
		void destroyGraphicsPipeline(hardware::GraphicsPipeline pipeline) final;
		void destroyBottomLevelAccelerationStructure(hardware::BottomLevelAccelerationStructure acceleration_structure) final;
		void destroyTopLevelAccelerationStructure(hardware::TopLevelAccelerationStructure acceleration_structure) final;
		void destroyRayTracePipeline(hardware::RayTracePipeline pipeline) final;
		void destroySwapChain(hardware::SwapChain swap_chain) final;

	public:
		bool isFlipped() final { return false; }
		Multisample getMaxSampleCount() final;

		uint32_t getNumSwapChainImages(hardware::SwapChain swap_chain) final;

		void setTextureSamplerWrapMode(hardware::Texture texture, SamplerWrapMode mode) final;
		void setTextureSamplerDepthCompare(hardware::Texture texture, bool enabled, DepthCompareFunc func) final;
		void generateTexture2DMipmaps(hardware::Texture texture) final;

	public:
		void *map(hardware::VertexBuffer vertex_buffer) final;
		void unmap(hardware::VertexBuffer vertex_buffer) final;

		void *map(hardware::IndexBuffer index_buffer) final;
		void unmap(hardware::IndexBuffer index_buffer) final;

		void *map(hardware::UniformBuffer uniform_buffer) final;
		void unmap(hardware::UniformBuffer uniform_buffer) final;

		void flush(hardware::BindSet bind_set) final;
		void flush(hardware::GraphicsPipeline pipeline) final;
		void flush(hardware::RayTracePipeline pipeline) final;

	public:
		bool acquire(
			hardware::SwapChain swap_chain,
			uint32_t *new_image
		) final;

		bool present(
			hardware::SwapChain swap_chain,
			uint32_t num_wait_command_buffers,
			const hardware::CommandBuffer *wait_command_buffers
		) final;

		void wait() final;
		bool wait(
			uint32_t num_wait_command_buffers,
			const hardware::CommandBuffer *wait_command_buffers
		) final;

	public:
		// bindings
		void bindUniformBuffer(
			hardware::BindSet bind_set,
			uint32_t binding,
			hardware::UniformBuffer uniform_buffer
		) final;

		void bindTexture(
			hardware::BindSet bind_set,
			uint32_t binding,
			hardware::Texture texture
		) final;

		void bindTexture(
			hardware::BindSet bind_set,
			uint32_t binding,
			hardware::Texture texture,
			uint32_t base_mip,
			uint32_t num_mips,
			uint32_t base_layer,
			uint32_t num_layers
		) final;

		void bindTopLevelAccelerationStructure(
			hardware::BindSet bind_set,
			uint32_t binding,
			hardware::TopLevelAccelerationStructure tlas
		) final;

		void bindStorageImage(
			hardware::BindSet bind_set,
			uint32_t binding,
			hardware::Texture texture
		) final;

	public:
		// raytrace pipeline state
		void clearBindSets(
			hardware::RayTracePipeline pipeline
		) final;

		void setBindSet(
			hardware::RayTracePipeline pipeline,
			uint8_t binding,
			hardware::BindSet bind_set
		) final;

		void clearRaygenShaders(
			hardware::RayTracePipeline pipeline
		) final;

		void addRaygenShader(
			hardware::RayTracePipeline pipeline,
			hardware::Shader shader
		) final;

		void clearHitGroupShaders(
			hardware::RayTracePipeline pipeline
		) final;

		void addHitGroupShader(
			hardware::RayTracePipeline pipeline,
			hardware::Shader intersection_shader,
			hardware::Shader anyhit_shader,
			hardware::Shader closesthit_shader
		) final;

		void clearMissShaders(
			hardware::RayTracePipeline pipeline
		) final;

		void addMissShader(
			hardware::RayTracePipeline pipeline,
			hardware::Shader shader
		) final;

	public:
		// pipeline state
		void clearPushConstants(
			hardware::GraphicsPipeline pipeline
		) final;

		void setPushConstants(
			hardware::GraphicsPipeline pipeline,
			uint8_t size,
			const void *data
		) final;

		void clearBindSets(
			hardware::GraphicsPipeline pipeline
		) final;

		void setBindSet(
			hardware::GraphicsPipeline pipeline,
			uint8_t binding,
			hardware::BindSet bind_set
		) final;

		void clearShaders(
			hardware::GraphicsPipeline pipeline
		) final;

		void setShader(
			hardware::GraphicsPipeline pipeline,
			ShaderType type,
			hardware::Shader shader
		) final;

		void clearVertexStreams(
			hardware::GraphicsPipeline pipeline
		) final;

		void setVertexStream(
			hardware::GraphicsPipeline pipeline,
			uint8_t binding,
			hardware::VertexBuffer vertex_buffer
		) final;

		void setViewport(
			hardware::GraphicsPipeline pipeline,
			int32_t x,
			int32_t y,
			uint32_t width,
			uint32_t height
		) final;

		void setScissor(
			hardware::GraphicsPipeline pipeline,
			int32_t x,
			int32_t y,
			uint32_t width,
			uint32_t height
		) final;

		void setPrimitiveType(
			hardware::GraphicsPipeline pipeline,
			RenderPrimitiveType type
		) final;

		void setCullMode(
			hardware::GraphicsPipeline pipeline,
			CullMode mode
		) final;

		void setDepthTest(
			hardware::GraphicsPipeline pipeline,
			bool enabled
		) final;

		void setDepthWrite(
			hardware::GraphicsPipeline pipeline,
			bool enabled
		) final;

		void setDepthCompareFunc(
			hardware::GraphicsPipeline pipeline,
			DepthCompareFunc func
		) final;

		void setBlending(
			hardware::GraphicsPipeline pipeline,
			bool enabled
		) final;

		void setBlendFactors(
			hardware::GraphicsPipeline pipeline,
			BlendFactor src_factor,
			BlendFactor dest_factor
		) final;

	public:
		// command buffers
		bool resetCommandBuffer(
			hardware::CommandBuffer command_buffer
		) final;

		bool beginCommandBuffer(
			hardware::CommandBuffer command_buffer
		) final;

		bool endCommandBuffer(
			hardware::CommandBuffer command_buffer
		) final;

		bool submit(
			hardware::CommandBuffer command_buffer
		) final;

		bool submitSyncked(
			hardware::CommandBuffer command_buffer,
			hardware::SwapChain wait_swap_chain
		) final;

		bool submitSyncked(
			hardware::CommandBuffer command_buffer,
			uint32_t num_wait_command_buffers,
			const hardware::CommandBuffer *wait_command_buffers
		) final;

	public:
		// render commands
		void beginRenderPass(
			hardware::CommandBuffer command_buffer,
			hardware::RenderPass render_pass,
			hardware::FrameBuffer frame_buffer
		) final;

		void beginRenderPass(
			hardware::CommandBuffer command_buffer,
			hardware::RenderPass render_pass,
			hardware::SwapChain swap_chain
		) final;

		void endRenderPass(
			hardware::CommandBuffer command_buffer
		) final;

		void drawIndexedPrimitiveInstanced(
			hardware::CommandBuffer command_buffer,
			hardware::GraphicsPipeline pipeline,
			hardware::IndexBuffer index_buffer,
			uint32_t num_indices,
			uint32_t base_index,
			int32_t base_vertex,
			uint32_t num_instances,
			uint32_t base_instance
		) final;

		void traceRays(
			hardware::CommandBuffer command_buffer,
			hardware::RayTracePipeline pipeline,
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
