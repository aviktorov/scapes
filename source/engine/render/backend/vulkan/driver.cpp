#include <cassert>

#include "render/backend/vulkan/driver.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"

namespace render::backend
{
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
				texture->width,
				texture->height,
				texture->depth,
				texture->num_mipmaps,
				texture->num_layers,
				texture->samples,
				texture->format,
				texture->tiling,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
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
					texture->width,
					texture->height,
					texture->depth,
					texture->num_mipmaps,
					texture->num_layers,
					vulkan::toPixelSize(format),
					texture->format,
					data,
					num_data_mipmaps,
					num_data_layers
				);
			}
			else
			{
				// Prepare the image to be used as a color attachment
				VulkanUtils::transitionImageLayout(
					context,
					texture->image,
					texture->format,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					0,
					texture->num_mipmaps,
					0,
					texture->num_layers
				);
			}
		}
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
		assert(type != BufferType::STATIC && "Only static buffers are implemented at the moment");
		assert(data != nullptr && "Invalid vertex data");
		assert(num_vertices != 0 && "Invalid vertex count");
		assert(vertex_size != 0 && "Invalid vertex size");
		assert(num_attributes <= vulkan::VertexBuffer::MAX_ATTRIBUTES && "Vertex attributes are limited to 16");

		VkDeviceSize buffer_size = vertex_size * num_vertices;
		
		vulkan::VertexBuffer *result = new vulkan::VertexBuffer();
		result->vertex_size = vertex_size;
		result->num_vertices = num_vertices;
		result->num_attributes = num_attributes;

		// Copy vertex attributes
		memcpy(result->attributes, attributes, num_attributes * sizeof(VertexAttribute));

		// Create vertex buffer
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
		assert(type != BufferType::STATIC && "Only static buffers are implemented at the moment");
		assert(data != nullptr && "Invalid index data");
		assert(num_indices != 0 && "Invalid index count");

		VkDeviceSize buffer_size = vulkan::toIndexSize(index_size) * num_indices;
		
		vulkan::IndexBuffer *result = new vulkan::IndexBuffer();
		result->type = vulkan::toIndexType(index_size);
		result->num_indices = num_indices;

		// Create vertex buffer
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
}
