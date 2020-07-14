#include "render/backend/vulkan/Driver.h"
#include "render/backend/vulkan/Context.h"
#include "render/backend/vulkan/Device.h"
#include "render/backend/vulkan/Platform.h"
#include "render/backend/vulkan/DescriptorSetLayoutCache.h"
#include "render/backend/vulkan/PipelineLayoutCache.h"
#include "render/backend/vulkan/PipelineCache.h"
#include "render/backend/vulkan/RenderPassCache.h"
#include "render/backend/vulkan/RenderPassBuilder.h"
#include "render/backend/vulkan/Utils.h"

#include <shaderc/shaderc.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

namespace render::backend::vulkan
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

	namespace helpers
	{
		static void createTextureData(const Device *device, Texture *texture, Format format, const void *data, int num_data_mipmaps, int num_data_layers)
		{
			VkImageUsageFlags usage_flags = Utils::getImageUsageFlags(texture->format);

			Utils::createImage(
				device,
				texture->type,
				texture->width, texture->height, texture->depth,
				texture->num_mipmaps, texture->num_layers,
				texture->samples, texture->format, texture->tiling,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | usage_flags,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				texture->flags,
				texture->image,
				texture->memory
			);

			VkImageLayout source_layout = VK_IMAGE_LAYOUT_UNDEFINED;

			if (data != nullptr)
			{
				// prepare for transfer
				Utils::transitionImageLayout(
					device,
					texture->image,
					texture->format,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					0, texture->num_mipmaps,
					0, texture->num_layers
				);

				// transfer data to GPU
				Utils::fillImage(
					device,
					texture->image,
					texture->width, texture->height, texture->depth,
					texture->num_mipmaps, texture->num_layers,
					Utils::getPixelSize(format),
					texture->format,
					data,
					num_data_mipmaps,
					num_data_layers
				);

				source_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			}

			// prepare for shader access
			Utils::transitionImageLayout(
				device,
				texture->image,
				texture->format,
				source_layout,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				0, texture->num_mipmaps,
				0, texture->num_layers
			);

			// create base view & sampler
			texture->view = Utils::createImageView(
				device,
				texture->image,
				texture->format,
				Utils::getImageAspectFlags(texture->format),
				Utils::getImageBaseViewType(texture->type, texture->flags, texture->num_layers),
				0, texture->num_mipmaps,
				0, texture->num_layers
			);

			texture->sampler = Utils::createSampler(device, 0, texture->num_mipmaps);
		}

		static void selectOptimalSwapChainSettings(const Device *device, SwapChain *swap_chain, uint32_t width, uint32_t height)
		{
			// Get surface capabilities
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->getPhysicalDevice(), swap_chain->surface, &swap_chain->surface_capabilities);

			// Select the best surface format
			uint32_t num_surface_formats = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device->getPhysicalDevice(), swap_chain->surface, &num_surface_formats, nullptr);
			assert(num_surface_formats != 0);

			std::vector<VkSurfaceFormatKHR> surface_formats(num_surface_formats);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device->getPhysicalDevice(), swap_chain->surface, &num_surface_formats, surface_formats.data());

			// Select the best format if the surface has no preferred format
			if (surface_formats.size() == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
			{
				swap_chain->surface_format = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
			}
			// Otherwise, select one of the available formats
			else
			{
				swap_chain->surface_format = surface_formats[0];
				for (const auto &surface_format : surface_formats)
				{
					if (surface_format.format == VK_FORMAT_B8G8R8A8_UNORM && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					{
						swap_chain->surface_format = surface_format;
						break;
					}
				}
			}

			// Select the best present mode
			uint32_t num_present_modes = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device->getPhysicalDevice(), swap_chain->surface, &num_present_modes, nullptr);
			assert(num_present_modes != 0);

			std::vector<VkPresentModeKHR> present_modes(num_present_modes);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device->getPhysicalDevice(), swap_chain->surface, &num_present_modes, present_modes.data());

			swap_chain->present_mode = VK_PRESENT_MODE_FIFO_KHR;
			for (const auto &present_mode : present_modes)
			{
				// Some drivers currently don't properly support FIFO present mode,
				// so we should prefer IMMEDIATE mode if MAILBOX mode is not available
				if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
					swap_chain->present_mode = present_mode;

				if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					swap_chain->present_mode = present_mode;
					break;
				}
			}

			const VkSurfaceCapabilitiesKHR &capabilities = swap_chain->surface_capabilities;

			// Select current swap extent if window manager doesn't allow to set custom extent
			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			{
				swap_chain->sizes = capabilities.currentExtent;
			}
			// Otherwise, manually set extent to match the min/max extent bounds
			else
			{
				swap_chain->sizes.width = std::clamp(
					width,
					capabilities.minImageExtent.width,
					capabilities.maxImageExtent.width
				);

				swap_chain->sizes.height = std::clamp(
					height,
					capabilities.minImageExtent.height,
					capabilities.maxImageExtent.height
				);
			}

			// Simply sticking to this minimum means that we may sometimes have to wait
			// on the driver to complete internal operations before we can acquire another image to render to.
			// Therefore it is recommended to request at least one more image than the minimum
			swap_chain->num_images = capabilities.minImageCount + 1;

			// We should also make sure to not exceed the maximum number of images while doing this,
			// where 0 is a special value that means that there is no maximum
			if(capabilities.maxImageCount > 0)
				swap_chain->num_images = std::min(swap_chain->num_images, capabilities.maxImageCount);
		}

		static bool createSwapChainObjects(const Device *device, SwapChain *swap_chain)
		{
			const VkSurfaceCapabilitiesKHR &capabilities = swap_chain->surface_capabilities;

			VkSwapchainCreateInfoKHR info = {};
			info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			info.surface = swap_chain->surface;
			info.minImageCount = swap_chain->num_images;
			info.imageFormat = swap_chain->surface_format.format;
			info.imageColorSpace = swap_chain->surface_format.colorSpace;
			info.imageExtent = swap_chain->sizes;
			info.imageArrayLayers = 1;
			info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			if (device->getGraphicsQueueFamily() != swap_chain->present_queue_family)
			{
				uint32_t families[] = { device->getGraphicsQueueFamily(), swap_chain->present_queue_family };
				info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				info.queueFamilyIndexCount = 2;
				info.pQueueFamilyIndices = families;
			}
			else
			{
				info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				info.queueFamilyIndexCount = 0;
				info.pQueueFamilyIndices = nullptr;
			}

			info.preTransform = capabilities.currentTransform;
			info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			info.presentMode = swap_chain->present_mode;
			info.clipped = VK_TRUE;
			info.oldSwapchain = VK_NULL_HANDLE;

			if (vkCreateSwapchainKHR(device->getDevice(), &info, nullptr, &swap_chain->swap_chain) != VK_SUCCESS)
			{
				std::cerr << "createSwapChainObjects(): vkCreateSwapchainKHR failed" << std::endl;
				return false;
			}

			// Get surface images
			vkGetSwapchainImagesKHR(device->getDevice(), swap_chain->swap_chain, &swap_chain->num_images, nullptr);
			assert(swap_chain->num_images != 0 && swap_chain->num_images < SwapChain::MAX_IMAGES);
			vkGetSwapchainImagesKHR(device->getDevice(), swap_chain->swap_chain, &swap_chain->num_images, swap_chain->images);

			// Create frame objects
			for (size_t i = 0; i < swap_chain->num_images; i++)
			{
				swap_chain->views[i] = Utils::createImageView(
					device,
					swap_chain->images[i],
					swap_chain->surface_format.format,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_VIEW_TYPE_2D
				);

				VkSemaphoreCreateInfo semaphore_info = {};
				semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				if (vkCreateSemaphore(device->getDevice(), &semaphore_info, nullptr, &swap_chain->image_available_gpu[i]) != VK_SUCCESS)
				{
					std::cerr << "createSwapChainObjects(): can't create 'image available' semaphore" << std::endl;
					return false;
				}
			}

			return true;
		}

		static void destroySwapChainObjects(const Device *device, SwapChain *swap_chain)
		{
			for (size_t i = 0; i < swap_chain->num_images; ++i)
			{
				vkDestroyImageView(device->getDevice(), swap_chain->views[i], nullptr);
				swap_chain->images[i] = VK_NULL_HANDLE;
				swap_chain->views[i] = VK_NULL_HANDLE;

				vkDestroySemaphore(device->getDevice(), swap_chain->image_available_gpu[i], nullptr);
				swap_chain->image_available_gpu[i] = VK_NULL_HANDLE;
			}

			vkDestroySwapchainKHR(device->getDevice(), swap_chain->swap_chain, nullptr);
			swap_chain->swap_chain = VK_NULL_HANDLE;
		}

		static void updateBindSetLayout(const Device *device, BindSet *bind_set, VkDescriptorSetLayout new_layout)
		{
			if (new_layout == bind_set->set_layout)
				return;

			bind_set->set_layout = new_layout;

			if (bind_set->set != VK_NULL_HANDLE)
				vkFreeDescriptorSets(device->getDevice(), device->getDescriptorPool(), 1, &bind_set->set);

			VkDescriptorSetAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			info.descriptorPool = device->getDescriptorPool();
			info.descriptorSetCount = 1;
			info.pSetLayouts = &new_layout;

			vkAllocateDescriptorSets(device->getDevice(), &info, &bind_set->set);
			assert(bind_set->set);

			for (uint8_t i = 0; i < BindSet::MAX_BINDINGS; ++i)
			{
				if (!bind_set->binding_used[i])
					continue;

				bind_set->binding_dirty[i] = true;
			}
		}
	}

	Driver::Driver(const char *application_name, const char *engine_name)
	{
		device = new Device();
		device->init(application_name, engine_name);

		context = new Context();
		descriptor_set_layout_cache = new DescriptorSetLayoutCache(device);
		pipeline_layout_cache = new PipelineLayoutCache(device, descriptor_set_layout_cache);
		pipeline_cache = new PipelineCache(device, pipeline_layout_cache);
		render_pass_cache = new RenderPassCache(device);
	}

	Driver::~Driver()
	{
		delete pipeline_cache;
		pipeline_cache = nullptr;

		delete pipeline_layout_cache;
		pipeline_layout_cache = nullptr;

		delete descriptor_set_layout_cache;
		descriptor_set_layout_cache = nullptr;

		delete render_pass_cache;
		render_pass_cache = nullptr;

		delete context;
		context = nullptr;

		if (device)
		{
			device->wait();
			device->shutdown();
		}

		delete device;
		device = nullptr;
	}

	backend::VertexBuffer *Driver::createVertexBuffer(
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
		assert(num_attributes <= VertexBuffer::MAX_ATTRIBUTES && "Vertex attributes are limited to 16");

		VkDeviceSize buffer_size = vertex_size * num_vertices;

		VertexBuffer *result = new VertexBuffer();
		result->vertex_size = vertex_size;
		result->num_vertices = num_vertices;
		result->num_attributes = num_attributes;

		// copy vertex attributes
		for (uint8_t i = 0; i < num_attributes; ++i)
		{
			result->attribute_formats[i] = Utils::getFormat(attributes[i].format);
			result->attribute_offsets[i] = attributes[i].offset;
		}

		// create vertex buffer
		Utils::createBuffer(
			device,
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			result->buffer,
			result->memory
		);

		// fill vertex buffer
		Utils::fillBuffer(
			device,
			result->buffer,
			buffer_size,
			data
		);

		return result;
	}

	backend::IndexBuffer *Driver::createIndexBuffer(
		BufferType type,
		IndexSize index_size,
		uint32_t num_indices,
		const void *data
	)
	{
		assert(type == BufferType::STATIC && "Only static buffers are implemented at the moment");
		assert(data != nullptr && "Invalid index data");
		assert(num_indices != 0 && "Invalid index count");

		VkDeviceSize buffer_size = Utils::getIndexSize(index_size) * num_indices;

		IndexBuffer *result = new IndexBuffer();
		result->type = Utils::getIndexType(index_size);
		result->num_indices = num_indices;

		// create index buffer
		Utils::createBuffer(
			device,
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			result->buffer,
			result->memory
		);

		// fill vertex buffer
		Utils::fillBuffer(
			device,
			result->buffer,
			buffer_size,
			data
		);

		return result;
	}

	backend::RenderPrimitive *Driver::createRenderPrimitive(
		RenderPrimitiveType type,
		const backend::VertexBuffer *vertex_buffer,
		const backend::IndexBuffer *index_buffer
	)
	{
		assert(vertex_buffer != nullptr && "Invalid vertex buffer");
		assert(index_buffer != nullptr && "Invalid vertex buffer");

		RenderPrimitive *result = new RenderPrimitive();

		result->topology = Utils::getPrimitiveTopology(type);
		result->vertex_buffer = static_cast<const VertexBuffer *>(vertex_buffer);
		result->index_buffer = static_cast<const IndexBuffer *>(index_buffer);

		return result;
	}

	backend::Texture *Driver::createTexture2D(
		uint32_t width,
		uint32_t height,
		uint32_t num_mipmaps,
		Format format,
		Multisample samples,
		const void *data,
		uint32_t num_data_mipmaps
	)
	{
		assert(width != 0 && height != 0 && "Invalid texture size");
		assert(num_mipmaps != 0 && "Invalid mipmap count");
		assert((data == nullptr) || (data != nullptr && num_data_mipmaps != 0) && "Invalid data mipmaps");
		assert((samples == Multisample::COUNT_1) || (samples != Multisample::COUNT_1 && num_mipmaps == 1));

		Texture *result = new Texture();
		result->type = VK_IMAGE_TYPE_2D;
		result->format = Utils::getFormat(format);
		result->width = width;
		result->height = height;
		result->depth = 1;
		result->num_mipmaps = num_mipmaps;
		result->num_layers = 1;
		result->samples = Utils::getSamples(samples);
		result->tiling = VK_IMAGE_TILING_OPTIMAL;
		result->flags = 0;

		helpers::createTextureData(device, result, format, data, num_data_mipmaps, 1);

		return result;
	}

	backend::Texture *Driver::createTexture2DArray(
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

		Texture *result = new Texture();
		result->type = VK_IMAGE_TYPE_2D;
		result->format = Utils::getFormat(format);
		result->width = width;
		result->height = height;
		result->depth = 1;
		result->num_mipmaps = num_mipmaps;
		result->num_layers = num_layers;
		result->samples = VK_SAMPLE_COUNT_1_BIT;
		result->tiling = VK_IMAGE_TILING_OPTIMAL;
		result->flags = 0;

		helpers::createTextureData(device, result, format, data, num_data_mipmaps, num_data_layers);

		return result;
	}

	backend::Texture *Driver::createTexture3D(
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

		Texture *result = new Texture();
		result->type = VK_IMAGE_TYPE_3D;
		result->format = Utils::getFormat(format);
		result->width = width;
		result->height = height;
		result->depth = depth;
		result->num_mipmaps = num_mipmaps;
		result->num_layers = 1;
		result->samples = VK_SAMPLE_COUNT_1_BIT;
		result->tiling = VK_IMAGE_TILING_OPTIMAL;
		result->flags = 0;

		helpers::createTextureData(device, result, format, data, num_data_mipmaps, 1);

		return result;
	}

	backend::Texture *Driver::createTextureCube(
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

		Texture *result = new Texture();
		result->type = VK_IMAGE_TYPE_2D;
		result->format = Utils::getFormat(format);
		result->width = width;
		result->height = height;
		result->depth = 1;
		result->num_mipmaps = num_mipmaps;
		result->num_layers = 6;
		result->samples = VK_SAMPLE_COUNT_1_BIT;
		result->tiling = VK_IMAGE_TILING_OPTIMAL;
		result->flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		helpers::createTextureData(device, result, format, data, num_data_mipmaps, 1);

		return result;
	}

	backend::FrameBuffer *Driver::createFrameBuffer(
		uint8_t num_attachments,
		const FrameBufferAttachment *attachments
	)
	{
		// TODO: check for equal sizes (color + depthstencil)

		FrameBuffer *result = new FrameBuffer();

		uint32_t width = 0;
		uint32_t height = 0;
		RenderPassBuilder builder;

		builder.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS);

		// add color attachments
		result->num_attachments = 0;
		for (uint8_t i = 0; i < num_attachments; ++i)
		{
			const FrameBufferAttachment &attachment = attachments[i];
			VkImageView view = VK_NULL_HANDLE;
			VkFormat format = VK_FORMAT_UNDEFINED;
			VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
			bool resolve = false;

			if (attachment.type == FrameBufferAttachmentType::COLOR)
			{
				const FrameBufferAttachment::Color &color = attachment.color;
				const Texture *color_texture = static_cast<const Texture *>(color.texture);
				VkImageAspectFlags flags = Utils::getImageAspectFlags(color_texture->format);

				view = Utils::createImageView(
					device,
					color_texture->image, color_texture->format,
					flags, VK_IMAGE_VIEW_TYPE_2D,
					color.base_mip, color.num_mips,
					color.base_layer, color.num_layers
				);

				width = std::max<int>(1, color_texture->width / (1 << color.base_mip));
				height = std::max<int>(1, color_texture->height / (1 << color.base_mip));
				format = color_texture->format;
				samples = color_texture->samples;

				resolve = color.resolve_attachment;

				if (color.resolve_attachment)
				{
					builder.addColorResolveAttachment(color_texture->format);
					builder.addColorResolveAttachmentReference(0, i);
				}
				else
				{
					builder.addColorAttachment(color_texture->format, color_texture->samples);
					builder.addColorAttachmentReference(0, i);
				}
			}
			else if (attachment.type == FrameBufferAttachmentType::DEPTH)
			{
				const FrameBufferAttachment::Depth &depth = attachment.depth;
				const Texture *depth_texture = static_cast<const Texture *>(depth.texture);
				VkImageAspectFlags flags = Utils::getImageAspectFlags(depth_texture->format);

				view = Utils::createImageView(
					device,
					depth_texture->image, depth_texture->format,
					flags, VK_IMAGE_VIEW_TYPE_2D
				);

				width = depth_texture->width;
				height = depth_texture->height;
				format = depth_texture->format;
				samples = depth_texture->samples;

				builder.addDepthStencilAttachment(depth_texture->format, depth_texture->samples);
				builder.setDepthStencilAttachmentReference(0, i);
			}
			else if (attachment.type == FrameBufferAttachmentType::SWAP_CHAIN_COLOR)
			{
				const FrameBufferAttachment::SwapChainColor &swap_chain_color = attachment.swap_chain_color;
				const SwapChain *swap_chain = static_cast<const SwapChain *>(swap_chain_color.swap_chain);
				VkImageAspectFlags flags = Utils::getImageAspectFlags(swap_chain->surface_format.format);

				view = Utils::createImageView(
					device,
					swap_chain->images[swap_chain_color.base_image], swap_chain->surface_format.format,
					flags, VK_IMAGE_VIEW_TYPE_2D
				);

				width = swap_chain->sizes.width;
				height = swap_chain->sizes.height;
				format = swap_chain->surface_format.format;

				resolve = swap_chain_color.resolve_attachment;

				if (swap_chain_color.resolve_attachment)
				{
					builder.addColorResolveAttachment(swap_chain->surface_format.format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
					builder.addColorResolveAttachmentReference(0, i);
				}
				else
				{
					builder.addColorAttachment(swap_chain->surface_format.format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
					builder.addColorAttachmentReference(0, i);
				}
			}

			result->attachments[result->num_attachments] = view;
			result->attachment_types[result->num_attachments] = attachment.type;
			result->attachment_formats[result->num_attachments] = format;
			result->attachment_resolve[result->num_attachments] = resolve;
			result->attachment_samples[result->num_attachments] = samples;
			result->num_attachments++;
		}

		// create dummy renderpass
		result->dummy_render_pass = builder.build(device->getDevice()); // TODO: move to render pass cache
		result->sizes.width = width;
		result->sizes.height = height;

		// create framebuffer
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = result->dummy_render_pass;
		framebufferInfo.attachmentCount = result->num_attachments;
		framebufferInfo.pAttachments = result->attachments;
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device->getDevice(), &framebufferInfo, nullptr, &result->framebuffer) != VK_SUCCESS)
		{
			// TODO: log error
			delete result;
			result = nullptr;
		}

		return result;
	}

	backend::CommandBuffer *Driver::createCommandBuffer(
		CommandBufferType type
	)
	{
		CommandBuffer *result = new CommandBuffer();
		result->level = Utils::getCommandBufferLevel(type);

		// Allocate commandbuffer
		VkCommandBufferAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandPool = device->getCommandPool();
		info.level = result->level;
		info.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(device->getDevice(), &info, &result->command_buffer) != VK_SUCCESS)
		{
			std::cerr << "Driver::createCommandBuffer(): can't allocate command buffer" << std::endl;
			destroyCommandBuffer(result);
			return nullptr;
		}

		// Create synchronization primitives
		VkSemaphoreCreateInfo semaphore_info = {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(device->getDevice(), &semaphore_info, nullptr, &result->rendering_finished_gpu) != VK_SUCCESS)
		{
			std::cerr << "Driver::createCommandBuffer(): can't create 'rendering finished' semaphore" << std::endl;
			destroyCommandBuffer(result);
			return nullptr;
		}

		// fences
		VkFenceCreateInfo fence_info = {};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateFence(device->getDevice(), &fence_info, nullptr, &result->rendering_finished_cpu) != VK_SUCCESS)
		{
			std::cerr << "Driver::createCommandBuffer(): can't create 'rendering finished' fence" << std::endl;
			destroyCommandBuffer(result);
			return nullptr;
		}

		return result;
	}

	backend::UniformBuffer *Driver::createUniformBuffer(
		BufferType type,
		uint32_t size,
		const void *data
	)
	{
		assert(type == BufferType::DYNAMIC && "Only dynamic buffers are implemented at the moment");
		assert(size != 0 && "Invalid size");

		UniformBuffer *result = new UniformBuffer();
		result->size = size;

		Utils::createBuffer(
			device,
			size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			result->buffer,
			result->memory
		);

		if (vmaMapMemory(device->getVRAMAllocator(), result->memory, &result->pointer) != VK_SUCCESS)
		{
			// TODO: log error
			delete result;
			return nullptr;
		}

		if (data != nullptr)
			memcpy(result->pointer, data, static_cast<size_t>(size));

		return result;
	}

	backend::Shader *Driver::createShaderFromSource(
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
			std::cerr << "Driver::createShaderFromSource(): can't compile shader at \"" << path << "\"" << std::endl;
			std::cerr << "\t" << shaderc_result_get_error_message(compilation_result);

			shaderc_result_release(compilation_result);
			shaderc_compile_options_release(options);
			shaderc_compiler_release(compiler);

			return nullptr;
		}

		size_t bytecode_size = shaderc_result_get_length(compilation_result);
		const uint32_t *bytecode_data = reinterpret_cast<const uint32_t*>(shaderc_result_get_bytes(compilation_result));

		Shader *result = new Shader();
		result->type = type;
		result->module = Utils::createShaderModule(device, bytecode_data, bytecode_size);

		shaderc_result_release(compilation_result);
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		return result;
	}

	backend::Shader *Driver::createShaderFromBytecode(
		ShaderType type,
		uint32_t size,
		const void *data
	)
	{
		assert(size != 0 && "Invalid size");
		assert(data != nullptr && "Invalid data");

		Shader *result = new Shader();
		result->type = type;
		result->module = Utils::createShaderModule(device, reinterpret_cast<const uint32_t *>(data), size);

		return result;
	}

	backend::BindSet *Driver::createBindSet()
	{
		BindSet *result = new BindSet();
		memset(result, 0, sizeof(BindSet));

		return result;
	}

	backend::SwapChain *Driver::createSwapChain(
		void *native_window,
		uint32_t width,
		uint32_t height
	)
	{
		assert(native_window != nullptr && "Invalid window");

		SwapChain *result = new SwapChain();

		// Create platform surface
		result->surface = Platform::createSurface(device, native_window);
		if (result->surface == VK_NULL_HANDLE)
		{
			std::cerr << "Driver::createSwapChain(): can't create platform surface" << std::endl;
			destroySwapChain(result);
			return false;
		}

		// Fetch present queue family
		result->present_queue_family = Utils::getPresentQueueFamily(
			device->getPhysicalDevice(),
			result->surface,
			device->getGraphicsQueueFamily()
		);

		// Get present queue
		vkGetDeviceQueue(device->getDevice(), result->present_queue_family, 0, &result->present_queue);
		if (result->present_queue == VK_NULL_HANDLE)
		{
			std::cerr << "Driver::createSwapChain(): can't get present queue from logical device" << std::endl;
			destroySwapChain(result);
			return false;
		}

		helpers::selectOptimalSwapChainSettings(device, result, width, height);
		helpers::createSwapChainObjects(device, result);

		return result;
	}

	void Driver::destroyVertexBuffer(backend::VertexBuffer *vertex_buffer)
	{
		if (vertex_buffer == nullptr)
			return;

		VertexBuffer *vk_vertex_buffer = static_cast<VertexBuffer *>(vertex_buffer);

		vmaDestroyBuffer(device->getVRAMAllocator(), vk_vertex_buffer->buffer, vk_vertex_buffer->memory);

		vk_vertex_buffer->buffer = VK_NULL_HANDLE;
		vk_vertex_buffer->memory = VK_NULL_HANDLE;

		delete vertex_buffer;
		vertex_buffer = nullptr;
	}

	void Driver::destroyIndexBuffer(backend::IndexBuffer *index_buffer)
	{
		if (index_buffer == nullptr)
			return;

		IndexBuffer *vk_index_buffer = static_cast<IndexBuffer *>(index_buffer);

		vmaDestroyBuffer(device->getVRAMAllocator(), vk_index_buffer->buffer, vk_index_buffer->memory);

		vk_index_buffer->buffer = VK_NULL_HANDLE;
		vk_index_buffer->memory = VK_NULL_HANDLE;

		delete index_buffer;
		index_buffer = nullptr;
	}

	void Driver::destroyRenderPrimitive(backend::RenderPrimitive *render_primitive)
	{
		if (render_primitive == nullptr)
			return;

		RenderPrimitive *vk_render_primitive = static_cast<RenderPrimitive *>(render_primitive);

		vk_render_primitive->vertex_buffer = nullptr;
		vk_render_primitive->index_buffer = nullptr;

		delete render_primitive;
		render_primitive = nullptr;
	}

	void Driver::destroyTexture(backend::Texture *texture)
	{
		if (texture == nullptr)
			return;

		Texture *vk_texture = static_cast<Texture *>(texture);

		vmaDestroyImage(device->getVRAMAllocator(), vk_texture->image, vk_texture->memory);

		vk_texture->image = VK_NULL_HANDLE;
		vk_texture->memory = VK_NULL_HANDLE;
		vk_texture->format = VK_FORMAT_UNDEFINED;

		delete texture;
		texture = nullptr;
	}

	void Driver::destroyFrameBuffer(backend::FrameBuffer *frame_buffer)
	{
		if (frame_buffer == nullptr)
			return;

		FrameBuffer *vk_frame_buffer = static_cast<FrameBuffer *>(frame_buffer);

		for (uint8_t i = 0; i < vk_frame_buffer->num_attachments; ++i)
		{
			vkDestroyImageView(device->getDevice(), vk_frame_buffer->attachments[i], nullptr);
			vk_frame_buffer->attachments[i] = VK_NULL_HANDLE;
		}

		vkDestroyFramebuffer(device->getDevice(), vk_frame_buffer->framebuffer, nullptr);
		vk_frame_buffer->framebuffer = VK_NULL_HANDLE;

		vkDestroyRenderPass(device->getDevice(), vk_frame_buffer->dummy_render_pass, nullptr);
		vk_frame_buffer->dummy_render_pass = VK_NULL_HANDLE;

		delete frame_buffer;
		frame_buffer = nullptr;
	}

	void Driver::destroyCommandBuffer(backend::CommandBuffer *command_buffer)
	{
		if (command_buffer == nullptr)
			return;

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);

		vkFreeCommandBuffers(device->getDevice(), device->getCommandPool(), 1, &vk_command_buffer->command_buffer);
		vk_command_buffer->command_buffer = VK_NULL_HANDLE;

		vkDestroySemaphore(device->getDevice(), vk_command_buffer->rendering_finished_gpu, nullptr);
		vk_command_buffer->rendering_finished_gpu = VK_NULL_HANDLE;

		vkDestroyFence(device->getDevice(), vk_command_buffer->rendering_finished_cpu, nullptr);
		vk_command_buffer->rendering_finished_cpu = VK_NULL_HANDLE;

		delete command_buffer;
		command_buffer = nullptr;
	}

	void Driver::destroyUniformBuffer(backend::UniformBuffer *uniform_buffer)
	{
		if (uniform_buffer == nullptr)
			return;

		UniformBuffer *vk_uniform_buffer = static_cast<UniformBuffer *>(uniform_buffer);

		vmaUnmapMemory(device->getVRAMAllocator(), vk_uniform_buffer->memory);
		vmaDestroyBuffer(device->getVRAMAllocator(), vk_uniform_buffer->buffer, vk_uniform_buffer->memory);

		vk_uniform_buffer->buffer = VK_NULL_HANDLE;
		vk_uniform_buffer->memory = VK_NULL_HANDLE;

		delete uniform_buffer;
		uniform_buffer = nullptr;
	}

	void Driver::destroyShader(backend::Shader *shader)
	{
		if (shader == nullptr)
			return;

		Shader *vk_shader = static_cast<Shader *>(shader);

		vkDestroyShaderModule(device->getDevice(), vk_shader->module, nullptr);
		vk_shader->module = VK_NULL_HANDLE;

		delete shader;
		shader = nullptr;
	}

	void Driver::destroyBindSet(backend::BindSet *bind_set)
	{
		if (bind_set == nullptr)
			return;

		BindSet *vk_bind_set = static_cast<BindSet *>(bind_set);

		for (uint32_t i = 0; i < BindSet::MAX_BINDINGS; ++i)
		{
			if (!vk_bind_set->binding_used[i])
				continue;

			VkDescriptorSetLayoutBinding &info = vk_bind_set->bindings[i];
			if (info.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				continue;

			BindSet::Data &data = vk_bind_set->binding_data[i];
			vkDestroyImageView(device->getDevice(), data.texture.view, nullptr);
		}

		if (vk_bind_set->set != VK_NULL_HANDLE)
			vkFreeDescriptorSets(device->getDevice(), device->getDescriptorPool(), 1, &vk_bind_set->set);

		delete bind_set;
		bind_set = nullptr;
	}

	void Driver::destroySwapChain(backend::SwapChain *swap_chain)
	{
		if (swap_chain == nullptr)
			return;

		SwapChain *vk_swap_chain = static_cast<SwapChain *>(swap_chain);

		helpers::destroySwapChainObjects(device, vk_swap_chain);

		vk_swap_chain->present_queue_family = 0xFFFF;
		vk_swap_chain->present_queue = VK_NULL_HANDLE;
		vk_swap_chain->present_mode = VK_PRESENT_MODE_FIFO_KHR;
		vk_swap_chain->sizes = {0, 0};

		vk_swap_chain->num_images = 0;
		vk_swap_chain->current_image = 0;

		// Destroy platform surface
		Platform::destroySurface(device, vk_swap_chain->surface);
		vk_swap_chain->surface = nullptr;

		delete swap_chain;
		swap_chain = nullptr;
	}

	Multisample Driver::getMaxSampleCount()
	{
		assert(device != nullptr && "Invalid device");

		VkSampleCountFlagBits samples = device->getMaxSampleCount();
		return Utils::getApiSamples(samples);
	}

	Format Driver::getOptimalDepthFormat()
	{
		assert(device != nullptr && "Invalid device");

		VkFormat format = Utils::selectOptimalDepthFormat(device->getPhysicalDevice());
		return Utils::getApiFormat(format);
	}

	Format Driver::getSwapChainImageFormat(const backend::SwapChain *swap_chain)
	{
		assert(swap_chain != nullptr && "Invalid swap chain");
		const SwapChain *vk_swap_chain = static_cast<const SwapChain *>(swap_chain);

		return Utils::getApiFormat(vk_swap_chain->surface_format.format);
	}

	uint32_t Driver::getNumSwapChainImages(const backend::SwapChain *swap_chain)
	{
		assert(swap_chain != nullptr && "Invalid swap chain");
		const SwapChain *vk_swap_chain = static_cast<const SwapChain *>(swap_chain);

		return vk_swap_chain->num_images;
	}

	void Driver::generateTexture2DMipmaps(backend::Texture *texture)
	{
		assert(texture != nullptr && "Invalid texture");

		Texture *vk_texture = static_cast<Texture *>(texture);

		// prepare for transfer
		Utils::transitionImageLayout(
			device,
			vk_texture->image,
			vk_texture->format,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0,
			vk_texture->num_mipmaps
		);

		// generate 2D mipmaps with linear filter
		Utils::generateImage2DMipmaps(
			device,
			vk_texture->image,
			vk_texture->format,
			vk_texture->width,
			vk_texture->height,
			vk_texture->num_mipmaps,
			vk_texture->format,
			VK_FILTER_LINEAR
		);

		// prepare for shader access
		Utils::transitionImageLayout(
			device,
			vk_texture->image,
			vk_texture->format,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			0,
			vk_texture->num_mipmaps
		);
	}

	void *Driver::map(backend::UniformBuffer *uniform_buffer)
	{
		assert(uniform_buffer != nullptr && "Invalid uniform buffer");
		// TODO: check DYNAMIC buffer type

		UniformBuffer *vk_uniform_buffer = static_cast<UniformBuffer *>(uniform_buffer);

		// NOTE: here we should call vkMapMemory but since it was called during UBO creation, we do nothing here.
		//       It's important to do a proper stress test to see if we can map all previously created UBOs.
		return vk_uniform_buffer->pointer;
	}

	void Driver::unmap(backend::UniformBuffer *uniform_buffer)
	{
		assert(uniform_buffer != nullptr && "Invalid uniform buffer");
		// TODO: check DYNAMIC buffer type

		// NOTE: here we should call vkUnmapMemory but let's try to keep all UBOs mapped to memory.
		//       It's important to do a proper stress test to see if we can map all previously created UBOs.
	}

	void Driver::wait()
	{
		assert(device != nullptr && "Invalid device");

		device->wait();
	}

	bool Driver::wait(
		uint32_t num_wait_command_buffers,
		backend::CommandBuffer * const *wait_command_buffers
	)
	{
		if (num_wait_command_buffers == 0)
			return true;

		std::vector<VkFence> wait_fences(num_wait_command_buffers);

		for (uint32_t i = 0; i < num_wait_command_buffers; ++i)
		{
			const CommandBuffer *vk_wait_command_buffer = static_cast<const CommandBuffer *>(wait_command_buffers[i]);
			wait_fences[i] = vk_wait_command_buffer->rendering_finished_cpu;
		}

		VkResult result = vkWaitForFences(device->getDevice(), num_wait_command_buffers, wait_fences.data(), VK_TRUE, UINT64_MAX);

		return result == VK_SUCCESS;
	}

	bool Driver::acquire(backend::SwapChain *swap_chain, uint32_t *new_image)
	{
		SwapChain *vk_swap_chain = static_cast<SwapChain *>(swap_chain);
		uint32_t current_image = vk_swap_chain->current_image;

		VkResult result = vkAcquireNextImageKHR(
			device->getDevice(),
			vk_swap_chain->swap_chain,
			std::numeric_limits<uint64_t>::max(),
			vk_swap_chain->image_available_gpu[current_image],
			VK_NULL_HANDLE,
			new_image
		);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
			return false;

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			// TODO: log fatal
			// runtime_error("Can't acquire swap chain image");
			return false;
		}

		return true;
	}

	bool Driver::present(backend::SwapChain *swap_chain, uint32_t num_wait_command_buffers, backend::CommandBuffer * const *wait_command_buffers)
	{
		SwapChain *vk_swap_chain = static_cast<SwapChain *>(swap_chain);
		uint32_t current_image = vk_swap_chain->current_image;

		std::vector<VkSemaphore> wait_semaphores(num_wait_command_buffers);
		std::vector<VkFence> wait_fences(num_wait_command_buffers);

		if (num_wait_command_buffers != 0 && wait_command_buffers != nullptr)
		{

			for (uint32_t i = 0; i < num_wait_command_buffers; ++i)
			{
				const CommandBuffer *vk_wait_command_buffer = static_cast<const CommandBuffer *>(wait_command_buffers[i]);
				wait_semaphores[i] = vk_wait_command_buffer->rendering_finished_gpu;
				wait_fences[i] = vk_wait_command_buffer->rendering_finished_cpu;
			}
		}

		VkPresentInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.swapchainCount = 1;
		info.pSwapchains = &vk_swap_chain->swap_chain;
		info.pImageIndices = &current_image;

		if (wait_semaphores.size())
		{
			info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());
			info.pWaitSemaphores = wait_semaphores.data();
		}

		Utils::transitionImageLayout(
			device,
			vk_swap_chain->images[current_image],
			vk_swap_chain->surface_format.format,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		);

		VkResult result = vkQueuePresentKHR(vk_swap_chain->present_queue, &info);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			return false;

		if (result != VK_SUCCESS)
		{
			// TODO: log fatal
			// runtime_error("Can't present swap chain image");
			return false;
		}

		if (wait_fences.size())
			vkWaitForFences(
				device->getDevice(),
				static_cast<uint32_t>(wait_fences.size()),
				wait_fences.data(),
				VK_TRUE,
				std::numeric_limits<uint64_t>::max()
			);

		vk_swap_chain->current_image++;
		vk_swap_chain->current_image %= vk_swap_chain->num_images;

		return true;
	}

	/*
	 */
	void Driver::bindUniformBuffer(
		backend::BindSet *bind_set,
		uint32_t binding,
		const backend::UniformBuffer *uniform_buffer
	)
	{
		assert(binding < BindSet::MAX_BINDINGS);

		if (bind_set == nullptr)
			return;

		BindSet *vk_bind_set = static_cast<BindSet *>(bind_set);
		const UniformBuffer *vk_uniform_buffer = static_cast<const UniformBuffer *>(uniform_buffer);
		
		VkDescriptorSetLayoutBinding &info = vk_bind_set->bindings[binding];
		BindSet::Data &data = vk_bind_set->binding_data[binding];

		if (vk_bind_set->binding_used[binding] && info.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			vkDestroyImageView(device->getDevice(), data.texture.view, nullptr);
			info = {};
			data = {};
		}

		vk_bind_set->binding_used[binding] = (vk_uniform_buffer != nullptr);
		vk_bind_set->binding_dirty[binding] = true;

		if (vk_uniform_buffer == nullptr)
			return;

		data.ubo.buffer = vk_uniform_buffer->buffer;
		data.ubo.size = vk_uniform_buffer->size;
		data.ubo.offset = 0;

		info.binding = binding;
		info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		info.descriptorCount = 1;
		info.stageFlags = VK_SHADER_STAGE_ALL; // TODO: allow for different shader stages
		info.pImmutableSamplers = nullptr;
	}

	void Driver::bindTexture(
		backend::BindSet *bind_set,
		uint32_t binding,
		const backend::Texture *texture
	)
	{
		assert(binding < BindSet::MAX_BINDINGS);

		if (bind_set == nullptr)
			return;

		BindSet *vk_bind_set = static_cast<BindSet *>(bind_set);
		const Texture *vk_texture = static_cast<const Texture *>(texture);

		uint32_t num_mipmaps = (vk_texture) ? vk_texture->num_mipmaps : 0;
		uint32_t num_layers = (vk_texture) ? vk_texture->num_layers : 0;

		bindTexture(bind_set, binding, texture, 0, num_mipmaps, 0, num_layers);
	}

	void Driver::bindTexture(
		backend::BindSet *bind_set,
		uint32_t binding,
		const backend::Texture *texture,
		uint32_t base_mip,
		uint32_t num_mipmaps,
		uint32_t base_layer,
		uint32_t num_layers
	)
	{
		assert(binding < BindSet::MAX_BINDINGS);

		if (bind_set == nullptr)
			return;

		BindSet *vk_bind_set = static_cast<BindSet *>(bind_set);
		const Texture *vk_texture = static_cast<const Texture *>(texture);

		VkDescriptorSetLayoutBinding &info = vk_bind_set->bindings[binding];
		BindSet::Data &data = vk_bind_set->binding_data[binding];

		if (vk_bind_set->binding_used[binding] && info.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			vkDestroyImageView(device->getDevice(), data.texture.view, nullptr);
			info = {};
			data = {};
		}

		VkImageView view = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;

		if (vk_texture)
		{
			view = Utils::createImageView(
				device,
				vk_texture->image,
				vk_texture->format,
				Utils::getImageAspectFlags(vk_texture->format),
				Utils::getImageBaseViewType(vk_texture->type, vk_texture->flags, num_layers),
				base_mip, num_mipmaps,
				base_layer, num_layers
			);
			sampler = vk_texture->sampler;
		}

		vk_bind_set->binding_used[binding] = (vk_texture != nullptr);
		vk_bind_set->binding_dirty[binding] = true;

		data.texture.view = view;
		data.texture.sampler = sampler;

		info.binding = binding;
		info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		info.descriptorCount = 1;
		info.stageFlags = VK_SHADER_STAGE_ALL; // TODO: allow for different shader stages
		info.pImmutableSamplers = nullptr;
	}

	/*
	 */
	void Driver::clearPushConstants()
	{
		context->clearPushConstants();
	}

	void Driver::setPushConstants(uint8_t size, const void *data)
	{
		context->setPushConstants(size, data);
	}

	void Driver::clearBindSets()
	{
		context->clearBindSets();
	}

	void Driver::allocateBindSets(uint8_t size)
	{
		context->allocateBindSets(size);
	}

	void Driver::pushBindSet(backend::BindSet *set)
	{
		BindSet *vk_set = static_cast<BindSet *>(set);
		context->pushBindSet(vk_set);
	}

	void Driver::setBindSet(uint32_t binding, backend::BindSet *set)
	{
		BindSet *vk_set = static_cast<BindSet *>(set);
		context->setBindSet(binding, vk_set);
	}

	void Driver::clearShaders()
	{
		context->clearShaders();
	}

	void Driver::setShader(ShaderType type, const backend::Shader *shader)
	{
		const Shader *vk_shader = static_cast<const Shader *>(shader);
		context->setShader(type, vk_shader);
	}

	/*
	 */
	void Driver::setViewport(float x, float y, float width, float height)
	{
		VkViewport viewport;
		viewport.x = x;
		viewport.y = y;
		viewport.width = width;
		viewport.height = height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		context->setViewport(viewport);
	}

	void Driver::setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
	{
		VkRect2D scissor;
		scissor.offset = { x, y };
		scissor.extent = { width, height };
		context->setScissor(scissor);
	}

	void Driver::setCullMode(CullMode mode)
	{
		VkCullModeFlags vk_mode = Utils::getCullMode(mode);
		context->setCullMode(vk_mode);
	}

	void Driver::setDepthTest(bool enabled)
	{
		context->setDepthTest(enabled);
	}

	void Driver::setDepthWrite(bool enabled)
	{
		context->setDepthWrite(enabled);
	}

	void Driver::setDepthCompareFunc(DepthCompareFunc func)
	{
		VkCompareOp vk_func = Utils::getDepthCompareFunc(func);
		context->setDepthCompareFunc(vk_func);
	}

	void Driver::setBlending(bool enabled)
	{
		context->setBlending(enabled);
	}

	void Driver::setBlendFactors(BlendFactor src_factor, BlendFactor dest_factor)
	{
		VkBlendFactor vk_src_factor = Utils::getBlendFactor(src_factor);
		VkBlendFactor vk_dest_factor = Utils::getBlendFactor(dest_factor);
		context->setBlendFactors(vk_src_factor, vk_dest_factor);
	}

	/*
	 */
	bool Driver::resetCommandBuffer(backend::CommandBuffer *command_buffer)
	{
		if (command_buffer == nullptr)
			return false;

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		if (vkResetCommandBuffer(vk_command_buffer->command_buffer, 0) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Driver::beginCommandBuffer(backend::CommandBuffer *command_buffer)
	{
		if (command_buffer == nullptr)
			return false;

		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		if (vkBeginCommandBuffer(vk_command_buffer->command_buffer, &info) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Driver::endCommandBuffer(backend::CommandBuffer *command_buffer)
	{
		if (command_buffer == nullptr)
			return false;

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		if (vkEndCommandBuffer(vk_command_buffer->command_buffer) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Driver::submit(backend::CommandBuffer *command_buffer)
	{
		if (command_buffer == nullptr)
			return false;
		
		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);

		VkSubmitInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &vk_command_buffer->command_buffer;

		vkResetFences(device->getDevice(), 1, &vk_command_buffer->rendering_finished_cpu);
		if (vkQueueSubmit(device->getGraphicsQueue(), 1, &info, vk_command_buffer->rendering_finished_cpu) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Driver::submitSyncked(backend::CommandBuffer *command_buffer, const backend::SwapChain *wait_swap_chain)
	{
		if (command_buffer == nullptr)
			return false;
		
		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);

		VkSubmitInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &vk_command_buffer->command_buffer;
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &vk_command_buffer->rendering_finished_gpu;

		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		if (wait_swap_chain != nullptr)
		{
			const SwapChain *vk_wait_swap_chain = static_cast<const SwapChain *>(wait_swap_chain);
			uint32_t current_image = vk_wait_swap_chain->current_image;

			info.waitSemaphoreCount = 1;
			info.pWaitSemaphores = &vk_wait_swap_chain->image_available_gpu[current_image];
			info.pWaitDstStageMask = &wait_stage;
		}

		vkResetFences(device->getDevice(), 1, &vk_command_buffer->rendering_finished_cpu);
		if (vkQueueSubmit(device->getGraphicsQueue(), 1, &info, vk_command_buffer->rendering_finished_cpu) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Driver::submitSyncked(backend::CommandBuffer *command_buffer, uint32_t num_wait_command_buffers, backend::CommandBuffer * const *wait_command_buffers)
	{
		if (command_buffer == nullptr)
			return false;
		
		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);

		VkSubmitInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &vk_command_buffer->command_buffer;
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &vk_command_buffer->rendering_finished_gpu;

		std::vector<VkSemaphore> wait_semaphores;
		std::vector<VkPipelineStageFlags> wait_stages;

		if (num_wait_command_buffers != 0 && wait_command_buffers != nullptr)
		{
			wait_semaphores.resize(num_wait_command_buffers);
			wait_stages.resize(num_wait_command_buffers);

			for (uint32_t i = 0; i < num_wait_command_buffers; ++i)
			{
				const CommandBuffer *vk_wait_command_buffer = static_cast<const CommandBuffer *>(wait_command_buffers[i]);
				wait_semaphores[i] = vk_wait_command_buffer->rendering_finished_gpu;
				wait_stages[i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}

			info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());
			info.pWaitSemaphores = wait_semaphores.data();
			info.pWaitDstStageMask = wait_stages.data();
		}

		vkResetFences(device->getDevice(), 1, &vk_command_buffer->rendering_finished_cpu);
		if (vkQueueSubmit(device->getGraphicsQueue(), 1, &info, vk_command_buffer->rendering_finished_cpu) != VK_SUCCESS)
			return false;

		return true;
	}

	void Driver::beginRenderPass(backend::CommandBuffer *command_buffer, const backend::FrameBuffer *frame_buffer, const RenderPassInfo *info)
	{
		assert(frame_buffer);

		if (command_buffer == nullptr)
			return;
		
		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		const FrameBuffer *vk_frame_buffer = static_cast<const FrameBuffer *>(frame_buffer);

		VkRenderPass render_pass = render_pass_cache->fetch(vk_frame_buffer, info);
		context->setRenderPass(render_pass);
		context->setFramebuffer(vk_frame_buffer);

		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = render_pass;
		render_pass_info.framebuffer = vk_frame_buffer->framebuffer;
		render_pass_info.renderArea.offset = {0, 0};
		render_pass_info.renderArea.extent = vk_frame_buffer->sizes;
		render_pass_info.clearValueCount = vk_frame_buffer->num_attachments;
		render_pass_info.pClearValues = reinterpret_cast<const VkClearValue *>(info->clear_values);

		vkCmdBeginRenderPass(vk_command_buffer->command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void Driver::endRenderPass(backend::CommandBuffer *command_buffer)
	{
		if (command_buffer == nullptr)
			return;

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		vkCmdEndRenderPass(vk_command_buffer->command_buffer);

		context->setRenderPass(VK_NULL_HANDLE);
	}

	void Driver::drawIndexedPrimitive(backend::CommandBuffer *command_buffer, const backend::RenderPrimitive *render_primitive, uint32_t base_index, int32_t vertex_offset)
	{
		if (command_buffer == nullptr)
			return;

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		const RenderPrimitive *vk_render_primitive = static_cast<const RenderPrimitive *>(render_primitive);

		std::vector<VkDescriptorSet> sets(context->getNumBindSets());
		std::vector<VkWriteDescriptorSet> writes;
		std::vector<VkDescriptorImageInfo> image_infos;
		std::vector<VkDescriptorBufferInfo> buffer_infos;

		writes.reserve(BindSet::MAX_BINDINGS * context->getNumBindSets());
		image_infos.reserve(BindSet::MAX_BINDINGS * context->getNumBindSets());
		buffer_infos.reserve(BindSet::MAX_BINDINGS * context->getNumBindSets());

		for (uint8_t i = 0; i < context->getNumBindSets(); ++i)
		{
			BindSet *bind_set = context->getBindSet(i);

			VkDescriptorSetLayout new_layout = descriptor_set_layout_cache->fetch(bind_set);
			helpers::updateBindSetLayout(device, bind_set, new_layout);

			sets[i] = bind_set->set;

			for (uint8_t j = 0; j < BindSet::MAX_BINDINGS; ++j)
			{
				if (!bind_set->binding_used[j])
					continue;

				if (!bind_set->binding_dirty[j])
					continue;

				VkDescriptorType descriptor_type = bind_set->bindings[j].descriptorType;
				const BindSet::Data &data = bind_set->binding_data[j];

				VkWriteDescriptorSet write_set = {};
				write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write_set.dstSet = bind_set->set;
				write_set.dstBinding = j;
				write_set.dstArrayElement = 0;

				switch (descriptor_type)
				{
					case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
					{
						VkDescriptorImageInfo info = {};
						info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						info.imageView = data.texture.view;
						info.sampler = data.texture.sampler;

						image_infos.push_back(info);
						write_set.pImageInfo = &image_infos[image_infos.size() - 1];
					}
					break;
					case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					{
						VkDescriptorBufferInfo info = {};
						info.buffer = data.ubo.buffer;
						info.offset = data.ubo.offset;
						info.range = data.ubo.size;

						buffer_infos.push_back(info);
						write_set.pBufferInfo = &buffer_infos[buffer_infos.size() - 1];
					}
					break;
					default:
					{
						assert(false && "Unsupported descriptor type");
					}
					break;
				}

				write_set.descriptorType = descriptor_type;
				write_set.descriptorCount = 1;

				writes.push_back(write_set);

				bind_set->binding_dirty[j] = false;
			}
		}

		if (writes.size() > 0)
			vkUpdateDescriptorSets(device->getDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

		VkPipelineLayout pipeline_layout = pipeline_layout_cache->fetch(context);
		VkPipeline pipeline = pipeline_cache->fetch(context, vk_render_primitive);

		vkCmdBindPipeline(vk_command_buffer->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		if (context->getPushConstantsSize() > 0)
			vkCmdPushConstants(vk_command_buffer->command_buffer, pipeline_layout, VK_SHADER_STAGE_ALL, 0, context->getPushConstantsSize(), context->getPushConstants());

		if (sets.size() > 0)
			vkCmdBindDescriptorSets(vk_command_buffer->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

		VkViewport viewport = context->getViewport();
		VkRect2D scissor = context->getScissor();

		vkCmdSetViewport(vk_command_buffer->command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(vk_command_buffer->command_buffer, 0, 1, &scissor);

		VkBuffer vertex_buffers[] = { vk_render_primitive->vertex_buffer->buffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(vk_command_buffer->command_buffer, 0, 1, vertex_buffers, offsets);
		vkCmdBindIndexBuffer(vk_command_buffer->command_buffer, vk_render_primitive->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);

		uint32_t num_indices = vk_render_primitive->index_buffer->num_indices;
		uint32_t num_instances = 1;
		uint32_t base_instance = 0;
		vkCmdDrawIndexed(vk_command_buffer->command_buffer, num_indices, num_instances, base_index, vertex_offset, base_instance);
	}

	void Driver::drawIndexedPrimitiveInstanced(
		backend::CommandBuffer *command_buffer,
		const backend::RenderPrimitive *render_primitive,
		const backend::VertexBuffer *instance_buffer,
		uint32_t num_instances,
		uint32_t offset
	)
	{
		if (command_buffer == nullptr)
			return;

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		const RenderPrimitive *vk_render_primitive = static_cast<const RenderPrimitive *>(render_primitive);
	}
}
