#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

#include "render/backend/vulkan/driver.h"
#include "shaderc/shaderc.h"

#include "VulkanContext.h"
#include "VulkanRenderPassBuilder.h"
#include "VulkanUtils.h"

namespace render::backend
{
	namespace shaderc
	{
		static shaderc_shader_kind vulkan_to_shaderc_kind(ShaderType type)
		{
			switch(type)
			{
				// Graphics pipeline
				case ShaderType::VERTEX: return shaderc_vertex_shader;
				case ShaderType::TESSELLATION_CONTROL: return shaderc_tess_control_shader;
				case ShaderType::TESSELLATION_EVALUATION: return shaderc_tess_evaluation_shader;
				case ShaderType::GEOMETRY: return shaderc_geometry_shader;
				case ShaderType::FRAGMENT: return shaderc_fragment_shader;

				// Compute pipeline
				case ShaderType::COMPUTE: return shaderc_compute_shader;

#if NV_EXTENSIONS
				// Raytrace pipeline
				case ShaderType::RAY_GENERATION: return shaderc_raygen_shader;
				case ShaderType::INTERSECTION: return shaderc_intersection_shader;
				case ShaderType::ANY_HIT: return shaderc_anyhit_shader;
				case ShaderType::CLOSEST_HIT: return shaderc_closesthit_shader;
				case ShaderType::MISS: return shaderc_miss_shader;
				case ShaderType::CALLABLE: return shaderc_callable_shader;
#endif
			}

			return shaderc_glsl_infer_from_source;
		}

		static shaderc_include_result *includeResolver(
			void *user_data,
			const char *requested_source,
			int type,
			const char *requesting_source,
			size_t include_depth
		)
		{
			shaderc_include_result *result = new shaderc_include_result();
			result->user_data = user_data;
			result->source_name = nullptr;
			result->source_name_length = 0;
			result->content = nullptr;
			result->content_length = 0;

			std::string target_dir = "";

			switch (type)
			{
				case shaderc_include_type_standard:
				{
					// TODO: remove this, not generic
					target_dir = "shaders/";
				}
				break;

				case shaderc_include_type_relative:
				{
					std::string_view source_path = requesting_source;
					size_t pos = source_path.find_last_of("/\\");

					if (pos != std::string_view::npos)
						target_dir = source_path.substr(0, pos + 1);
				}
				break;
			}

			std::string target_path = target_dir + std::string(requested_source);

			std::ifstream file(target_path, std::ios::ate | std::ios::binary);

			if (!file.is_open())
			{
				std::cerr << "shaderc::include_resolver(): can't load include at \"" << target_path << "\"" << std::endl;
				return result;
			}

			size_t fileSize = static_cast<size_t>(file.tellg());
			char *buffer = new char[fileSize];

			file.seekg(0);
			file.read(buffer, fileSize);
			file.close();

			char *path = new char[target_path.size() + 1];
			memcpy(path, target_path.c_str(), target_path.size());
			path[target_path.size()] = '\x0';

			result->source_name = path;
			result->source_name_length = target_path.size() + 1;
			result->content = buffer;
			result->content_length = fileSize;

			return result;
		}

		static void includeResultReleaser(void *userData, shaderc_include_result *result)
		{
			delete result->source_name;
			delete result->content;
			delete result;
		}
	}

	namespace vulkan
	{
		static VkFormat toFormat(Format format)
		{
			static VkFormat supported_formats[static_cast<int>(Format::MAX)] =
			{
				VK_FORMAT_UNDEFINED,

				// 8-bit formats
				VK_FORMAT_R8_UNORM, VK_FORMAT_R8_SNORM, VK_FORMAT_R8_UINT, VK_FORMAT_R8_SINT,
				VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8_SNORM, VK_FORMAT_R8G8_UINT, VK_FORMAT_R8G8_SINT,
				VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8_SNORM, VK_FORMAT_R8G8B8_UINT, VK_FORMAT_R8G8B8_SINT,
				VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_SINT,

				// 16-bit formats
				VK_FORMAT_R16_UNORM, VK_FORMAT_R16_SNORM, VK_FORMAT_R16_UINT, VK_FORMAT_R16_SINT, VK_FORMAT_R16_SFLOAT,
				VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16_SNORM, VK_FORMAT_R16G16_UINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_SFLOAT,
				VK_FORMAT_R16G16B16_UNORM, VK_FORMAT_R16G16B16_SNORM, VK_FORMAT_R16G16B16_UINT, VK_FORMAT_R16G16B16_SINT, VK_FORMAT_R16G16B16_SFLOAT,
				VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SFLOAT,

				// 32-bit formats
				VK_FORMAT_R32_UINT, VK_FORMAT_R32_SINT, VK_FORMAT_R32_SFLOAT,
				VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32_SINT, VK_FORMAT_R32G32_SFLOAT,
				VK_FORMAT_R32G32B32_UINT, VK_FORMAT_R32G32B32_SINT, VK_FORMAT_R32G32B32_SFLOAT,
				VK_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R32G32B32A32_SINT, VK_FORMAT_R32G32B32A32_SFLOAT,

				// depth formats
				VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
			};

			return supported_formats[static_cast<int>(format)];
		}

		static VkImageAspectFlags toImageAspectFlags(VkFormat format)
		{
			if (format == VK_FORMAT_UNDEFINED)
				return 0;

			switch (format)
			{
				case VK_FORMAT_D16_UNORM:
				case VK_FORMAT_D32_SFLOAT: return VK_IMAGE_ASPECT_DEPTH_BIT;
				case VK_FORMAT_D16_UNORM_S8_UINT:
				case VK_FORMAT_D24_UNORM_S8_UINT:
				case VK_FORMAT_D32_SFLOAT_S8_UINT: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			}

			return VK_IMAGE_ASPECT_COLOR_BIT;
		}

		static VkImageViewType toImageBaseViewType(VkImageType type, VkImageCreateFlags flags, uint32_t num_layers)
		{
			if ((type == VK_IMAGE_TYPE_2D) && (num_layers == 1) && (flags == 0))
				return VK_IMAGE_VIEW_TYPE_2D;

			if ((type == VK_IMAGE_TYPE_2D) && (num_layers > 1) && (flags == 0))
				return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

			if ((type == VK_IMAGE_TYPE_3D) && (num_layers == 1) && (flags == 0))
				return VK_IMAGE_VIEW_TYPE_3D;

			if (type == VK_IMAGE_TYPE_2D && num_layers == 6 && (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT))
				return VK_IMAGE_VIEW_TYPE_CUBE;

			return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		}

		static uint8_t toPixelSize(Format format)
		{
			static uint8_t supported_formats[static_cast<int>(Format::MAX)] =
			{
				0,

				// 8-bit formats
				1, 1, 1, 1,
				2, 2, 2, 2,
				3, 3, 3, 3,
				4, 4, 4, 4,

				// 16-bit formats
				2, 2, 2, 2, 2,
				4, 4, 4, 4, 4,
				6, 6, 6, 6, 6,
				8, 8, 8, 8, 8,

				// 32-bit formats
				4, 4, 4,
				8, 8, 8,
				12, 12, 12,
				16, 16, 16,

				// depth formats
				2, 3, 4, 4, 5,
			};

			return supported_formats[static_cast<int>(format)];
		}

		static VkIndexType toIndexType(IndexSize size)
		{
			static VkIndexType supported_sizes[static_cast<int>(IndexSize::MAX)] =
			{
				VK_INDEX_TYPE_UINT16, VK_INDEX_TYPE_UINT32,
			};

			return supported_sizes[static_cast<int>(size)];
		}

		static uint8_t toIndexSize(IndexSize size)
		{
			static uint8_t supported_sizes[static_cast<int>(IndexSize::MAX)] =
			{
				2, 4
			};

			return supported_sizes[static_cast<int>(size)];
		}

		static VkPrimitiveTopology toPrimitiveTopology(RenderPrimitiveType type)
		{
			static VkPrimitiveTopology supported_types[static_cast<int>(RenderPrimitiveType::MAX)] =
			{
				// points
				VK_PRIMITIVE_TOPOLOGY_POINT_LIST,

				// lines
				VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_PRIMITIVE_TOPOLOGY_PATCH_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,

				// triangles
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_PATCH_LIST, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,

				// quads
				VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
			};

			return supported_types[static_cast<int>(type)];
		}

		static void createTextureData(const VulkanContext *context, Texture *texture, Format format, const void *data, int num_data_mipmaps, int num_data_layers)
		{
			VulkanUtils::createImage(
				context,
				texture->type,
				texture->width, texture->height, texture->depth,
				texture->num_mipmaps, texture->num_layers,
				texture->samples, texture->format, texture->tiling,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				texture->flags,
				texture->image,
				texture->memory
			);

			if (data != nullptr)
			{
				VulkanUtils::fillImage(
					context,
					texture->image,
					texture->width, texture->height, texture->depth,
					texture->num_mipmaps, texture->num_layers,
					vulkan::toPixelSize(format),
					texture->format,
					data,
					num_data_mipmaps,
					num_data_layers
				);
			}

			// Prepare the image for shader access
			VulkanUtils::transitionImageLayout(
				context,
				texture->image,
				texture->format,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				0, texture->num_mipmaps,
				0, texture->num_layers
			);

			// Create image view & sampler
			texture->view = VulkanUtils::createImageView(
				context,
				texture->image,
				texture->format,
				toImageAspectFlags(texture->format),
				toImageBaseViewType(texture->type, texture->flags, texture->num_layers),
				0, texture->num_mipmaps,
				0, texture->num_layers
			);

			texture->sampler = VulkanUtils::createSampler(context, 0, texture->num_mipmaps);
		}
	}

	VulkanDriver::VulkanDriver(const char *application_name, const char *engine_name)
	{
		context = new VulkanContext();
		context->init(application_name, engine_name);
	}

	VulkanDriver::~VulkanDriver()
	{
		if (context)
		{
			context->wait();
			context->shutdown();
		}

		delete context;
		context = nullptr;
	}

	VertexBuffer *VulkanDriver::createVertexBuffer(
		BufferType type,
		uint16_t vertex_size,
		uint32_t num_vertices,
		uint8_t num_attributes,
		const VertexAttribute *attributes,
		const void *data
	)
	{
		assert(type == BufferType::STATIC && "Only static buffers are implemented at the moment");
		assert(data != nullptr && "Invalid vertex data");
		assert(num_vertices != 0 && "Invalid vertex count");
		assert(vertex_size != 0 && "Invalid vertex size");
		assert(num_attributes <= vulkan::VertexBuffer::MAX_ATTRIBUTES && "Vertex attributes are limited to 16");

		VkDeviceSize buffer_size = vertex_size * num_vertices;
		
		vulkan::VertexBuffer *result = new vulkan::VertexBuffer();
		result->vertex_size = vertex_size;
		result->num_vertices = num_vertices;
		result->num_attributes = num_attributes;

		// copy vertex attributes
		memcpy(result->attributes, attributes, num_attributes * sizeof(VertexAttribute));

		// create vertex buffer
		VulkanUtils::createDeviceLocalBuffer(
			context,
			buffer_size,
			data,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			result->buffer,
			result->memory
		);

		return result;
	}

	IndexBuffer *VulkanDriver::createIndexBuffer(
		BufferType type,
		IndexSize index_size,
		uint32_t num_indices,
		const void *data
	)
	{
		assert(type == BufferType::STATIC && "Only static buffers are implemented at the moment");
		assert(data != nullptr && "Invalid index data");
		assert(num_indices != 0 && "Invalid index count");

		VkDeviceSize buffer_size = vulkan::toIndexSize(index_size) * num_indices;
		
		vulkan::IndexBuffer *result = new vulkan::IndexBuffer();
		result->type = vulkan::toIndexType(index_size);
		result->num_indices = num_indices;

		// create index buffer
		VulkanUtils::createDeviceLocalBuffer(
			context,
			buffer_size,
			data,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			result->buffer,
			result->memory
		);

		return result;
	}

	RenderPrimitive *VulkanDriver::createRenderPrimitive(
		RenderPrimitiveType type,
		const VertexBuffer *vertex_buffer,
		const IndexBuffer *index_buffer
	)
	{
		assert(vertex_buffer != nullptr && "Invalid vertex buffer");
		assert(index_buffer != nullptr && "Invalid vertex buffer");

		vulkan::RenderPrimitive *result = new vulkan::RenderPrimitive();

		result->topology = vulkan::toPrimitiveTopology(type);
		result->vertex_buffer = static_cast<const vulkan::VertexBuffer *>(vertex_buffer);
		result->index_buffer = static_cast<const vulkan::IndexBuffer *>(index_buffer);

		return result;
	}

	Texture *VulkanDriver::createTexture2D(
		uint32_t width,
		uint32_t height,
		uint32_t num_mipmaps,
		Format format,
		const void *data,
		uint32_t num_data_mipmaps
	)
	{
		assert(width != 0 && height != 0 && "Invalid texture size");
		assert(num_mipmaps != 0 && "Invalid mipmap count");
		assert((data == nullptr) || (data != nullptr && num_data_mipmaps != 0) && "Invalid data mipmaps");

		vulkan::Texture *result = new vulkan::Texture();
		result->type = VK_IMAGE_TYPE_2D;
		result->format = vulkan::toFormat(format);
		result->width = width;
		result->height = height;
		result->depth = 1;
		result->num_mipmaps = num_mipmaps;
		result->num_layers = 1;
		result->samples = VK_SAMPLE_COUNT_1_BIT;
		result->tiling = VK_IMAGE_TILING_OPTIMAL;
		result->flags = 0;

		vulkan::createTextureData(context, result, format, data, num_data_mipmaps, 1);

		return result;
	}

	Texture *VulkanDriver::createTexture2DArray(
		uint32_t width,
		uint32_t height,
		uint32_t num_mipmaps,
		uint32_t num_layers,
		Format format,
		const void *data,
		uint32_t num_data_mipmaps,
		uint32_t num_data_layers
	)
	{
		assert(width != 0 && height != 0 && "Invalid texture size");
		assert(num_layers != 0 && "Invalid layer count");
		assert(num_mipmaps != 0 && "Invalid mipmap count");
		assert((data == nullptr) || (data != nullptr && num_data_mipmaps != 0) && "Invalid data mipmaps");
		assert((data == nullptr) || (data != nullptr && num_data_layers != 0) && "Invalid data layers");

		vulkan::Texture *result = new vulkan::Texture();
		result->type = VK_IMAGE_TYPE_2D;
		result->format = vulkan::toFormat(format);
		result->width = width;
		result->height = height;
		result->depth = 1;
		result->num_mipmaps = num_mipmaps;
		result->num_layers = num_layers;
		result->samples = VK_SAMPLE_COUNT_1_BIT;
		result->tiling = VK_IMAGE_TILING_OPTIMAL;
		result->flags = 0;

		vulkan::createTextureData(context, result, format, data, num_data_mipmaps, num_data_layers);

		return result;
	}

	Texture *VulkanDriver::createTexture3D(
		uint32_t width,
		uint32_t height,
		uint32_t depth,
		uint32_t num_mipmaps,
		Format format,
		const void *data,
		uint32_t num_data_mipmaps
	)
	{
		assert(width != 0 && height != 0 && depth != 0 && "Invalid texture size");
		assert(num_mipmaps != 0 && "Invalid mipmap count");
		assert((data == nullptr) || (data != nullptr && num_data_mipmaps != 0) && "Invalid data mipmaps");

		vulkan::Texture *result = new vulkan::Texture();
		result->type = VK_IMAGE_TYPE_3D;
		result->format = vulkan::toFormat(format);
		result->width = width;
		result->height = height;
		result->depth = depth;
		result->num_mipmaps = num_mipmaps;
		result->num_layers = 1;
		result->samples = VK_SAMPLE_COUNT_1_BIT;
		result->tiling = VK_IMAGE_TILING_OPTIMAL;
		result->flags = 0;

		vulkan::createTextureData(context, result, format, data, num_data_mipmaps, 1);

		return result;
	}

	Texture *VulkanDriver::createTextureCube(
		uint32_t width,
		uint32_t height,
		uint32_t num_mipmaps,
		Format format,
		const void *data,
		uint32_t num_data_mipmaps
	)
	{
		assert(width != 0 && height != 0 && "Invalid texture size");
		assert(width == height && "Texture must be square");
		assert(num_mipmaps != 0 && "Invalid mipmap count");
		assert((data == nullptr) || (data != nullptr && num_data_mipmaps != 0) && "Invalid data mipmaps");

		vulkan::Texture *result = new vulkan::Texture();
		result->type = VK_IMAGE_TYPE_2D;
		result->format = vulkan::toFormat(format);
		result->width = width;
		result->height = height;
		result->depth = 1;
		result->num_mipmaps = num_mipmaps;
		result->num_layers = 6;
		result->samples = VK_SAMPLE_COUNT_1_BIT;
		result->tiling = VK_IMAGE_TILING_OPTIMAL;
		result->flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		vulkan::createTextureData(context, result, format, data, num_data_mipmaps, 1);

		return result;
	}

	FrameBuffer *VulkanDriver::createFrameBuffer(
		uint8_t num_color_attachments,
		const FrameBufferColorAttachment *color_attachments,
		const FrameBufferDepthStencilAttachment *depthstencil_attachment
	)
	{
		assert((depthstencil_attachment != nullptr && num_color_attachments == 0) || (num_color_attachments != 0) && "Invalid attachments");

		// TODO: check for equal sizes (color + depthstencil)

		vulkan::FrameBuffer *result = new vulkan::FrameBuffer();

		VkImageView attachments[vulkan::FrameBuffer::MAX_COLOR_ATTACHMENTS + 1];
		uint8_t num_attachments = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		VulkanRenderPassBuilder builder = VulkanRenderPassBuilder(context);

		builder.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS);

		// add color attachments
		result->num_color_attachments = num_color_attachments;
		for (uint8_t i = 0; i < num_color_attachments; ++i)
		{
			const FrameBufferColorAttachment &attachment = color_attachments[i];
			const vulkan::Texture *texture = static_cast<const vulkan::Texture *>(attachment.texture);

			VkImageView view = VulkanUtils::createImageView(
				context,
				texture->image, texture->format,
				VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D,
				attachment.base_mip, attachment.num_mips,
				attachment.base_layer, attachment.num_layers
			);

			result->color_attachments[i].view = view;
			attachments[num_attachments++] = view;

			width = std::max<int>(1, texture->width / (1 << attachment.base_mip));
			height = std::max<int>(1, texture->height / (1 << attachment.base_mip));

			builder.addColorAttachment(texture->format, texture->samples);
			builder.addColorAttachmentReference(0, i);
		}

		// add depthstencil attachment
		if (depthstencil_attachment != nullptr)
		{
			const vulkan::Texture *texture = static_cast<const vulkan::Texture *>(depthstencil_attachment->texture);

			VkImageAspectFlags flags = vulkan::toImageAspectFlags(texture->format);
			assert((flags & VK_IMAGE_ASPECT_DEPTH_BIT) && "Invalid depthstencil attachment format");

			VkImageView view = VulkanUtils::createImageView(
				context,
				texture->image, texture->format,
				flags, VK_IMAGE_VIEW_TYPE_2D
			);

			result->depthstencil_attachment.view = view;
			attachments[num_attachments++] = view;

			builder.addDepthStencilAttachment(texture->format, texture->samples);
			builder.setDepthStencilAttachmentReference(0, num_color_attachments);
		}

		// create dummy renderpass
		result->dummy_render_pass = builder.build(); // TODO: move to render pass cache

		// create framebuffer
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = result->dummy_render_pass;
		framebufferInfo.attachmentCount = num_attachments;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(context->getDevice(), &framebufferInfo, nullptr, &result->framebuffer) != VK_SUCCESS)
		{
			// TODO: log error
			delete result;
			result = nullptr;
		}

		return result;
	}

	UniformBuffer *VulkanDriver::createUniformBuffer(
		BufferType type,
		uint32_t size,
		const void *data
	)
	{
		assert(type == BufferType::DYNAMIC && "Only dynamic buffers are implemented at the moment");
		assert(size != 0 && "Invalid size");

		vulkan::UniformBuffer *result = new vulkan::UniformBuffer();
		result->size = size;

		VulkanUtils::createBuffer(
			context,
			size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			result->buffer,
			result->memory
		);

		if (vkMapMemory(context->getDevice(), result->memory, 0, size, 0, &result->pointer) != VK_SUCCESS)
		{
			// TODO: log error
			delete result;
			return nullptr;
		}

		if (data != nullptr)
			memcpy(result->pointer, data, static_cast<size_t>(size));

		return result;
	}

	Shader *VulkanDriver::createShaderFromSource(
		ShaderType type,
		uint32_t size,
		const char *data,
		const char *path
	)
	{
		// convert GLSL/HLSL code to SPIR-V bytecode
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();

		// set compile options
		shaderc_compile_options_set_include_callbacks(options, shaderc::includeResolver, shaderc::includeResultReleaser, nullptr);

		// compile shader
		shaderc_compilation_result_t compilation_result = shaderc_compile_into_spv(
			compiler,
			data, size,
			shaderc_glsl_infer_from_source,
			path,
			"main",
			options
		);

		if (shaderc_result_get_compilation_status(compilation_result) != shaderc_compilation_status_success)
		{
			std::cerr << "VulkanDriver::createShaderFromSource(): can't compile shader at \"" << path << "\"" << std::endl;
			std::cerr << "\t" << shaderc_result_get_error_message(compilation_result);

			shaderc_result_release(compilation_result);
			shaderc_compile_options_release(options);
			shaderc_compiler_release(compiler);

			return nullptr;
		}

		size_t bytecode_size = shaderc_result_get_length(compilation_result);
		const uint32_t *bytecode_data = reinterpret_cast<const uint32_t*>(shaderc_result_get_bytes(compilation_result));

		vulkan::Shader *result = new vulkan::Shader();
		result->type = type;
		result->module = VulkanUtils::createShaderModule(context, bytecode_data, bytecode_size);

		shaderc_result_release(compilation_result);
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		return result;
	}

	Shader *VulkanDriver::createShaderFromBytecode(
		ShaderType type,
		uint32_t size,
		const void *data
	)
	{
		assert(size != 0 && "Invalid size");
		assert(data != nullptr && "Invalid data");

		vulkan::Shader *result = new vulkan::Shader();
		result->type = type;
		result->module = VulkanUtils::createShaderModule(context, reinterpret_cast<const uint32_t *>(data), size);

		return result;
	}


	void VulkanDriver::destroyVertexBuffer(VertexBuffer *vertex_buffer)
	{
		if (vertex_buffer == nullptr)
			return;


		vulkan::VertexBuffer *vk_vertex_buffer = static_cast<vulkan::VertexBuffer *>(vertex_buffer);

		vkDestroyBuffer(context->getDevice(), vk_vertex_buffer->buffer, nullptr);
		vkFreeMemory(context->getDevice(), vk_vertex_buffer->memory, nullptr);

		vk_vertex_buffer->buffer = VK_NULL_HANDLE;
		vk_vertex_buffer->memory = VK_NULL_HANDLE;

		delete vertex_buffer;
		vertex_buffer = nullptr;
	}

	void VulkanDriver::destroyIndexBuffer(IndexBuffer *index_buffer)
	{
		if (index_buffer == nullptr)
			return;

		vulkan::IndexBuffer *vk_index_buffer = static_cast<vulkan::IndexBuffer *>(index_buffer);

		vkDestroyBuffer(context->getDevice(), vk_index_buffer->buffer, nullptr);
		vkFreeMemory(context->getDevice(), vk_index_buffer->memory, nullptr);

		vk_index_buffer->buffer = VK_NULL_HANDLE;
		vk_index_buffer->memory = VK_NULL_HANDLE;

		delete index_buffer;
		index_buffer = nullptr;
	}

	void VulkanDriver::destroyRenderPrimitive(RenderPrimitive *render_primitive)
	{
		if (render_primitive == nullptr)
			return;

		vulkan::RenderPrimitive *vk_render_primitive = static_cast<vulkan::RenderPrimitive *>(render_primitive);

		vk_render_primitive->vertex_buffer = nullptr;
		vk_render_primitive->index_buffer = nullptr;

		delete render_primitive;
		render_primitive = nullptr;
	}

	void VulkanDriver::destroyTexture(Texture *texture)
	{
		if (texture == nullptr)
			return;

		vulkan::Texture *vk_texture = static_cast<vulkan::Texture *>(texture);

		vkDestroyImage(context->getDevice(), vk_texture->image, nullptr);
		vkFreeMemory(context->getDevice(), vk_texture->memory, nullptr);

		vk_texture->image = VK_NULL_HANDLE;
		vk_texture->memory = VK_NULL_HANDLE;
		vk_texture->format = VK_FORMAT_UNDEFINED;

		delete texture;
		texture = nullptr;
	}

	void VulkanDriver::destroyFrameBuffer(FrameBuffer *frame_buffer)
	{
		if (frame_buffer == nullptr)
			return;

		vulkan::FrameBuffer *vk_frame_buffer = static_cast<vulkan::FrameBuffer *>(frame_buffer);

		for (uint8_t i = 0; i < vk_frame_buffer->num_color_attachments; ++i)
		{
			vkDestroyImageView(context->getDevice(), vk_frame_buffer->color_attachments[i].view, nullptr);
			vk_frame_buffer->color_attachments[i].view = VK_NULL_HANDLE;
		}

		vkDestroyImageView(context->getDevice(), vk_frame_buffer->depthstencil_attachment.view, nullptr);
		vk_frame_buffer->depthstencil_attachment.view = VK_NULL_HANDLE;

		vkDestroyFramebuffer(context->getDevice(), vk_frame_buffer->framebuffer, nullptr);
		vk_frame_buffer->framebuffer = VK_NULL_HANDLE;

		vkDestroyRenderPass(context->getDevice(), vk_frame_buffer->dummy_render_pass, nullptr);
		vk_frame_buffer->dummy_render_pass = VK_NULL_HANDLE;

		delete frame_buffer;
		frame_buffer = nullptr;
	}

	void VulkanDriver::destroyUniformBuffer(UniformBuffer *uniform_buffer)
	{
		if (uniform_buffer == nullptr)
			return;

		vulkan::UniformBuffer *vk_uniform_buffer = static_cast<vulkan::UniformBuffer *>(uniform_buffer);

		vkDestroyBuffer(context->getDevice(), vk_uniform_buffer->buffer, nullptr);
		vkFreeMemory(context->getDevice(), vk_uniform_buffer->memory, nullptr);

		vk_uniform_buffer->buffer = VK_NULL_HANDLE;
		vk_uniform_buffer->memory = VK_NULL_HANDLE;

		delete uniform_buffer;
		uniform_buffer = nullptr;
	}

	void VulkanDriver::destroyShader(Shader *shader)
	{
		if (shader == nullptr)
			return;

		vulkan::Shader *vk_shader = static_cast<vulkan::Shader *>(shader);

		vkDestroyShaderModule(context->getDevice(), vk_shader->module, nullptr);
		vk_shader->module = VK_NULL_HANDLE;

		delete shader;
		shader = nullptr;
	}

	void VulkanDriver::generateTexture2DMipmaps(Texture *texture)
	{
		assert(texture != nullptr && "Invalid texture");

		vulkan::Texture *vk_texture = static_cast<vulkan::Texture *>(texture);

		VulkanUtils::generateImage2DMipmaps(
			context,
			vk_texture->image,
			vk_texture->format,
			vk_texture->width,
			vk_texture->height,
			vk_texture->num_mipmaps,
			vk_texture->format,
			VK_FILTER_LINEAR
		);
	}

	void *VulkanDriver::map(UniformBuffer *uniform_buffer)
	{
		assert(uniform_buffer != nullptr && "Invalid uniform buffer");
		// TODO: check DYNAMIC buffer type

		vulkan::UniformBuffer *vk_uniform_buffer = static_cast<vulkan::UniformBuffer *>(uniform_buffer);

		// NOTE: here we should call vkMapMemory but since it was called during UBO creation, we do nothing here.
		//       It's important to do a proper stress test to see if we can map all previously created UBOs.
		return vk_uniform_buffer->pointer;
	}

	void VulkanDriver::unmap(UniformBuffer *uniform_buffer)
	{
		assert(uniform_buffer != nullptr && "Invalid uniform buffer");
		// TODO: check DYNAMIC buffer type

		// NOTE: here we should call vkUnmapMemory but let's try to keep all UBOs mapped to memory.
		//       It's important to do a proper stress test to see if we can map all previously created UBOs.
	}

	void VulkanDriver::wait()
	{
		assert(context != nullptr && "Invalid context");

		context->wait();
	}

	void VulkanDriver::beginRenderPass(const FrameBuffer *frame_buffer)
	{

	}

	void VulkanDriver::endRenderPass()
	{
	}

	void VulkanDriver::bindUniformBuffer(uint32_t unit, const UniformBuffer *uniform_buffer)
	{

	}

	void VulkanDriver::bindTexture(uint32_t unit, const Texture *texture)
	{

	}

	void VulkanDriver::bindShader(const Shader *shader)
	{

	}

	void VulkanDriver::drawIndexedPrimitive(const RenderPrimitive *render_primitive)
	{

	}

	void VulkanDriver::drawIndexedPrimitiveInstanced(
		const RenderPrimitive *render_primitive,
		const VertexBuffer *instance_buffer,
		uint32_t num_instances,
		uint32_t offset
	)
	{

	}
}
