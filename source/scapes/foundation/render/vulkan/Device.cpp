#include "render/vulkan/Device.h"
#include "render/vulkan/Context.h"
#include "render/vulkan/Platform.h"
#include "render/vulkan/DescriptorSetLayoutCache.h"
#include "render/vulkan/ImageViewCache.h"
#include "render/vulkan/PipelineLayoutCache.h"
#include "render/vulkan/PipelineCache.h"
#include "render/vulkan/RenderPassBuilder.h"
#include "render/vulkan/Utils.h"

#include <scapes/foundation/profiler/Profiler.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

namespace scapes::foundation::render::vulkan
{
	namespace helpers
	{
		static void createTextureData(const Context *context, Texture *texture, Format format, const void *data, int num_data_mipmaps, int num_data_layers)
		{
			VkImageUsageFlags usage_flags = Utils::getImageUsageFlags(texture->format);

			Utils::createImage(
				context,
				texture->type,
				texture->width, texture->height, texture->depth,
				texture->num_mipmaps, texture->num_layers,
				texture->samples, texture->format, texture->tiling,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | usage_flags,
				texture->flags,
				texture->image,
				texture->memory
			);

			VkImageLayout source_layout = VK_IMAGE_LAYOUT_UNDEFINED;

			if (data != nullptr)
			{
				// prepare for transfer
				Utils::transitionImageLayout(
					context,
					texture->image,
					texture->format,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					0, texture->num_mipmaps,
					0, texture->num_layers
				);

				// transfer data to GPU
				Utils::fillImage(
					context,
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
				context,
				texture->image,
				texture->format,
				source_layout,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				0, texture->num_mipmaps,
				0, texture->num_layers
			);

			// create base sampler & view cache
			texture->sampler = Utils::createSampler(context, 0, texture->num_mipmaps);
			texture->image_view_cache = new ImageViewCache(context);
		}

		static void selectOptimalSwapChainSettings(const Context *context, SwapChain *swap_chain)
		{
			// Get surface capabilities
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->getPhysicalDevice(), swap_chain->surface, &swap_chain->surface_capabilities);

			// Select the best surface format
			uint32_t num_surface_formats = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(context->getPhysicalDevice(), swap_chain->surface, &num_surface_formats, nullptr);
			assert(num_surface_formats != 0);

			std::vector<VkSurfaceFormatKHR> surface_formats(num_surface_formats);
			vkGetPhysicalDeviceSurfaceFormatsKHR(context->getPhysicalDevice(), swap_chain->surface, &num_surface_formats, surface_formats.data());

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
			vkGetPhysicalDeviceSurfacePresentModesKHR(context->getPhysicalDevice(), swap_chain->surface, &num_present_modes, nullptr);
			assert(num_present_modes != 0);

			std::vector<VkPresentModeKHR> present_modes(num_present_modes);
			vkGetPhysicalDeviceSurfacePresentModesKHR(context->getPhysicalDevice(), swap_chain->surface, &num_present_modes, present_modes.data());

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
				swap_chain->sizes = capabilities.currentExtent;
			else
				swap_chain->sizes = capabilities.minImageExtent;

			// Simply sticking to this minimum means that we may sometimes have to wait
			// on the driver to complete internal operations before we can acquire another image to render to.
			// Therefore it is recommended to request at least one more image than the minimum
			swap_chain->num_images = capabilities.minImageCount + 1;

			// We should also make sure to not exceed the maximum number of images while doing this,
			// where 0 is a special value that means that there is no maximum
			if(capabilities.maxImageCount > 0)
				swap_chain->num_images = std::min(swap_chain->num_images, capabilities.maxImageCount);
		}

		static bool createSwapChainObjects(const Context *context, SwapChain *swap_chain)
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
			info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			info.preTransform = capabilities.currentTransform;
			info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			info.presentMode = swap_chain->present_mode;
			info.clipped = VK_TRUE;
			info.oldSwapchain = VK_NULL_HANDLE;

			uint32_t families[2] = { context->getGraphicsQueueFamily(), swap_chain->present_queue_family };
			if (context->getGraphicsQueueFamily() != swap_chain->present_queue_family)
			{
				info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				info.queueFamilyIndexCount = 2;
				info.pQueueFamilyIndices = families;
			}

			if (vkCreateSwapchainKHR(context->getDevice(), &info, nullptr, &swap_chain->swap_chain) != VK_SUCCESS)
			{
				std::cerr << "createSwapChainObjects(): vkCreateSwapchainKHR failed" << std::endl;
				return false;
			}

			// Get surface images
			vkGetSwapchainImagesKHR(context->getDevice(), swap_chain->swap_chain, &swap_chain->num_images, nullptr);
			assert(swap_chain->num_images != 0 && swap_chain->num_images < SwapChain::MAX_IMAGES);
			vkGetSwapchainImagesKHR(context->getDevice(), swap_chain->swap_chain, &swap_chain->num_images, swap_chain->images);

			// Create dummy render pass
			RenderPassBuilder render_pass_builder;
			swap_chain->dummy_render_pass = render_pass_builder
				.addAttachment(
					swap_chain->surface_format.format,
					VK_SAMPLE_COUNT_1_BIT,
					VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					VK_ATTACHMENT_STORE_OP_DONT_CARE,
					VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					VK_ATTACHMENT_STORE_OP_DONT_CARE,
					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
				)
				.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
				.addColorAttachmentReference(0, 0)
				.build(context->getDevice());

			// Create frame objects (semaphores, image views, framebuffers)
			for (size_t i = 0; i < swap_chain->num_images; i++)
			{
				Utils::transitionImageLayout(
					context,
					swap_chain->images[i],
					swap_chain->surface_format.format,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
				);

				VkSemaphoreCreateInfo semaphore_info = {};
				semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				if (vkCreateSemaphore(context->getDevice(), &semaphore_info, nullptr, &swap_chain->image_available_gpu[i]) != VK_SUCCESS)
				{
					std::cerr << "createSwapChainObjects(): can't create 'image available' semaphore" << std::endl;
					return false;
				}

				VkImageViewCreateInfo view_info = {};
				view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				view_info.image = swap_chain->images[i];
				view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
				view_info.format = swap_chain->surface_format.format;
				view_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
				view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				view_info.subresourceRange.baseMipLevel = 0;
				view_info.subresourceRange.levelCount = 1;
				view_info.subresourceRange.baseArrayLayer = 0;
				view_info.subresourceRange.layerCount = 1;

				if (vkCreateImageView(context->getDevice(), &view_info, nullptr, &swap_chain->image_views[i]) != VK_SUCCESS)
				{
					std::cerr << "createSwapChainObjects(): can't create swap chain image view" << std::endl;
					return false;
				}

				VkFramebufferCreateInfo framebuffer_info = {};
				framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebuffer_info.renderPass = swap_chain->dummy_render_pass;
				framebuffer_info.attachmentCount = 1;
				framebuffer_info.pAttachments = swap_chain->image_views + i;
				framebuffer_info.width = swap_chain->sizes.width;
				framebuffer_info.height = swap_chain->sizes.height;
				framebuffer_info.layers = 1;

				if (vkCreateFramebuffer(context->getDevice(), &framebuffer_info, nullptr, &swap_chain->frame_buffers[i]) != VK_SUCCESS)
				{
					std::cerr << "createSwapChainObjects(): can't create framebuffer for swap chain image view" << std::endl;
					return false;
				}
			}

			return true;
		}

		static void destroySwapChainObjects(const Context *context, SwapChain *swap_chain)
		{
			for (size_t i = 0; i < swap_chain->num_images; ++i)
			{
				swap_chain->images[i] = VK_NULL_HANDLE;

				vkDestroySemaphore(context->getDevice(), swap_chain->image_available_gpu[i], nullptr);
				swap_chain->image_available_gpu[i] = VK_NULL_HANDLE;

				vkDestroyImageView(context->getDevice(), swap_chain->image_views[i], nullptr);
				swap_chain->image_views[i] = VK_NULL_HANDLE;

				vkDestroyFramebuffer(context->getDevice(), swap_chain->frame_buffers[i], nullptr);
				swap_chain->frame_buffers[i] = VK_NULL_HANDLE;
			}

			vkDestroyRenderPass(context->getDevice(), swap_chain->dummy_render_pass, nullptr);
			swap_chain->dummy_render_pass = VK_NULL_HANDLE;

			vkDestroySwapchainKHR(context->getDevice(), swap_chain->swap_chain, nullptr);
			swap_chain->swap_chain = VK_NULL_HANDLE;
		}

		static void updateBindSetLayout(const Context *context, BindSet *bind_set, VkDescriptorSetLayout new_layout)
		{
			if (new_layout == bind_set->set_layout)
				return;

			bind_set->set_layout = new_layout;

			if (bind_set->set != VK_NULL_HANDLE)
				vkFreeDescriptorSets(context->getDevice(), context->getDescriptorPool(), 1, &bind_set->set);

			VkDescriptorSetAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			info.descriptorPool = context->getDescriptorPool();
			info.descriptorSetCount = 1;
			info.pSetLayouts = &new_layout;

			vkAllocateDescriptorSets(context->getDevice(), &info, &bind_set->set);
			assert(bind_set->set);

			for (uint8_t i = 0; i < BindSet::MAX_BINDINGS; ++i)
			{
				if (!bind_set->binding_used[i])
					continue;

				bind_set->binding_dirty[i] = true;
			}
		}

		static void destroyAccelerationStructure(const Context *context, AccelerationStructure *acceleration_structure)
		{
			vkDestroyAccelerationStructureKHR(context->getDevice(), acceleration_structure->acceleration_structure, nullptr);
			vmaDestroyBuffer(context->getVRAMAllocator(), acceleration_structure->buffer, acceleration_structure->memory);

			memset(acceleration_structure, 0, sizeof(BottomLevelAccelerationStructure));

			delete acceleration_structure;
			acceleration_structure = nullptr;
		}
	}

	Device::Device(const char *application_name, const char *engine_name)
	{
		context = new Context();
		context->init(application_name, engine_name);

		descriptor_set_layout_cache = new DescriptorSetLayoutCache(context);
		pipeline_layout_cache = new PipelineLayoutCache(context, descriptor_set_layout_cache);
		pipeline_cache = new PipelineCache(context, pipeline_layout_cache);
	}

	Device::~Device()
	{
		delete pipeline_cache;
		pipeline_cache = nullptr;

		delete pipeline_layout_cache;
		pipeline_layout_cache = nullptr;

		delete descriptor_set_layout_cache;
		descriptor_set_layout_cache = nullptr;

		if (context)
		{
			vkDeviceWaitIdle(context->getDevice());
			context->shutdown();
		}

		delete context;
		context = nullptr;
	}

	render::VertexBuffer Device::createVertexBuffer(
		BufferType type,
		uint16_t vertex_size,
		uint32_t num_vertices,
		uint8_t num_attributes,
		const VertexAttribute *attributes,
		const void *data
	)
	{
		assert(num_vertices != 0 && "Invalid vertex count");
		assert(vertex_size != 0 && "Invalid vertex size");
		assert(num_attributes <= VertexBuffer::MAX_ATTRIBUTES && "Vertex attributes are limited to 16");

		VkDeviceSize buffer_size = vertex_size * num_vertices;

		VertexBuffer *result = new VertexBuffer();
		result->type = type;
		result->vertex_size = vertex_size;
		result->num_vertices = num_vertices;
		result->num_attributes = num_attributes;

		// copy vertex attributes
		for (uint8_t i = 0; i < num_attributes; ++i)
		{
			result->attribute_formats[i] = Utils::getFormat(attributes[i].format);
			result->attribute_offsets[i] = attributes[i].offset;
		}

		VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_UNKNOWN;

		if (type == BufferType::STATIC)
		{
			usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
		}
		else if (type == BufferType::DYNAMIC)
			memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		// create vertex buffer
		Utils::createBuffer(context, buffer_size, usage_flags, memory_usage, result->buffer, result->memory);

		if (data)
		{
			if (type == BufferType::STATIC)
				Utils::fillDeviceLocalBuffer(context, result->buffer, buffer_size, data);
			else if (type == BufferType::DYNAMIC)
				Utils::fillHostVisibleBuffer(context, result->memory, buffer_size, data);
		}

		return reinterpret_cast<render::VertexBuffer>(result);
	}

	render::IndexBuffer Device::createIndexBuffer(
		BufferType type,
		IndexFormat index_format,
		uint32_t num_indices,
		const void *data
	)
	{
		assert(num_indices != 0 && "Invalid index count");

		VkDeviceSize buffer_size = Utils::getIndexSize(index_format) * num_indices;

		IndexBuffer *result = new IndexBuffer();
		result->type = type;
		result->index_type = Utils::getIndexType(index_format);
		result->num_indices = num_indices;

		VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_UNKNOWN;

		if (type == BufferType::STATIC)
		{
			usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
		}
		else if (type == BufferType::DYNAMIC)
			memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		// create index buffer
		Utils::createBuffer(context, buffer_size, usage_flags, memory_usage, result->buffer, result->memory);

		if (data)
		{
			if (type == BufferType::STATIC)
				Utils::fillDeviceLocalBuffer(context, result->buffer, buffer_size, data);
			else if (type == BufferType::DYNAMIC)
				Utils::fillHostVisibleBuffer(context, result->memory, buffer_size, data);
		}

		return reinterpret_cast<render::IndexBuffer>(result);
	}

	render::Texture Device::createTexture2D(
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

		Texture *result = new Texture();
		result->type = VK_IMAGE_TYPE_2D;
		result->format = Utils::getFormat(format);
		result->width = width;
		result->height = height;
		result->depth = 1;
		result->num_mipmaps = num_mipmaps;
		result->num_layers = 1;
		result->samples = VK_SAMPLE_COUNT_1_BIT;
		result->tiling = VK_IMAGE_TILING_OPTIMAL;
		result->flags = 0;

		helpers::createTextureData(context, result, format, data, num_data_mipmaps, 1);

		return reinterpret_cast<render::Texture>(result);
	}

	render::Texture Device::createTexture2DMultisample(
		uint32_t width,
		uint32_t height,
		Format format,
		Multisample samples
	)
	{
		assert(width != 0 && height != 0 && "Invalid texture size");

		Texture *result = new Texture();
		result->type = VK_IMAGE_TYPE_2D;
		result->format = Utils::getFormat(format);
		result->width = width;
		result->height = height;
		result->depth = 1;
		result->num_mipmaps = 1;
		result->num_layers = 1;
		result->samples = Utils::getSamples(samples);
		result->tiling = VK_IMAGE_TILING_OPTIMAL;
		result->flags = 0;

		helpers::createTextureData(context, result, format, nullptr, 1, 1);

		return reinterpret_cast<render::Texture>(result);
	}

	render::Texture Device::createTexture2DArray(
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

		helpers::createTextureData(context, result, format, data, num_data_mipmaps, num_data_layers);

		return reinterpret_cast<render::Texture>(result);
	}

	render::Texture Device::createTexture3D(
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

		helpers::createTextureData(context, result, format, data, num_data_mipmaps, 1);

		return reinterpret_cast<render::Texture>(result);
	}

	render::Texture Device::createTextureCube(
		uint32_t size,
		uint32_t num_mipmaps,
		Format format,
		const void *data,
		uint32_t num_data_mipmaps
	)
	{
		assert(size != 0 && "Invalid texture size");
		assert(num_mipmaps != 0 && "Invalid mipmap count");
		assert((data == nullptr) || (data != nullptr && num_data_mipmaps != 0) && "Invalid data mipmaps");

		Texture *result = new Texture();
		result->type = VK_IMAGE_TYPE_2D;
		result->format = Utils::getFormat(format);
		result->width = size;
		result->height = size;
		result->depth = 1;
		result->num_mipmaps = num_mipmaps;
		result->num_layers = 6;
		result->samples = VK_SAMPLE_COUNT_1_BIT;
		result->tiling = VK_IMAGE_TILING_OPTIMAL;
		result->flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		helpers::createTextureData(context, result, format, data, num_data_mipmaps, 1);

		return reinterpret_cast<render::Texture>(result);
	}

	render::StorageImage Device::createStorageImage(
		uint32_t width,
		uint32_t height,
		Format format
	)
	{
		assert(width != 0 && height != 0 && "Invalid texture size");

		Texture *result = new Texture();
		result->type = VK_IMAGE_TYPE_2D;
		result->format = Utils::getFormat(format);
		result->width = width;
		result->height = height;
		result->depth = 1;
		result->num_mipmaps = 1;
		result->num_layers = 1;
		result->samples = VK_SAMPLE_COUNT_1_BIT;
		result->tiling = VK_IMAGE_TILING_OPTIMAL;
		result->flags = 0;
		result->image_view_cache = new ImageViewCache(context);
		result->sampler = Utils::createSampler(context, 0, result->num_mipmaps);

		VkImageUsageFlags usage_flags = Utils::getImageUsageFlags(result->format) | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

		Utils::createImage(
			context,
			result->type,
			result->width, result->height, result->depth,
			result->num_mipmaps, result->num_layers,
			result->samples, result->format, result->tiling,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | usage_flags,
			result->flags,
			result->image,
			result->memory
		);

		Utils::transitionImageLayout(
			context,
			result->image,
			result->format,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			0, result->num_mipmaps,
			0, result->num_layers
		);

		return reinterpret_cast<render::StorageImage>(result);
	}

	render::FrameBuffer Device::createFrameBuffer(
		uint32_t num_attachments,
		const FrameBufferAttachment *attachments
	)
	{
		assert(num_attachments <= FrameBuffer::MAX_ATTACHMENTS);
		assert(num_attachments == 0 || (num_attachments && attachments));

		FrameBuffer *result = new FrameBuffer();

		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t num_layers = 1;

		// add color attachments
		result->num_attachments = num_attachments;
		for (uint32_t i = 0; i < num_attachments; ++i)
		{
			const FrameBufferAttachment &input_attachment = attachments[i];
			const Texture *texture = reinterpret_cast<const Texture *>(input_attachment.texture);

			result->attachment_views[i] = texture->image_view_cache->fetch(texture, input_attachment.base_mip, 1, input_attachment.base_layer, input_attachment.num_layers);
			result->attachment_formats[i] = texture->format;
			result->attachment_samples[i] = texture->samples;

			width = std::max<int>(1, texture->width >> input_attachment.base_mip);
			height = std::max<int>(1, texture->height >> input_attachment.base_mip);

			num_layers = std::max<int>(num_layers, input_attachment.num_layers);
		}

		result->sizes.width = width;
		result->sizes.height = height;

		result->num_layers = num_layers;

		return reinterpret_cast<render::FrameBuffer>(result);
	}

	render::RenderPass Device::createRenderPass(
		uint32_t num_attachments,
		const RenderPassAttachment *attachments,
		const RenderPassDescription &description
	)
	{
		assert(num_attachments <= RenderPass::MAX_ATTACHMENTS);
		assert(num_attachments == 0 || (num_attachments && attachments));

		RenderPassBuilder builder;
		RenderPass *result = new RenderPass();

		result->num_attachments = num_attachments;
		for (uint32_t i = 0; i < num_attachments; ++i)
		{
			const RenderPassAttachment &input_attachment = attachments[i];

			VkFormat format = Utils::getFormat(input_attachment.format);
			VkSampleCountFlagBits samples = Utils::getSamples(input_attachment.samples);
			VkAttachmentLoadOp load_op = Utils::getLoadOp(input_attachment.load_op);
			VkAttachmentStoreOp store_op = Utils::getStoreOp(input_attachment.store_op);

			result->attachment_formats[i] = format;
			result->attachment_samples[i] = samples;
			result->attachment_load_ops[i] = load_op;
			result->attachment_store_ops[i] = store_op;
			memcpy(&result->attachment_clear_values[i], &input_attachment.clear_value, sizeof(VkClearValue));

			builder.addAttachment(format, samples, load_op, store_op, load_op, store_op);
		}

		builder.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS); // TODO: add different binding points

		result->num_color_attachments = description.num_color_attachments;
		for (uint32_t i = 0; i < description.num_color_attachments; ++i)
		{
			uint32_t index = description.color_attachments[i];
			VkSampleCountFlagBits samples = result->attachment_samples[index];

			builder.addColorAttachmentReference(0, index);

			if (description.resolve_attachments)
				builder.addColorResolveAttachmentReference(0, description.resolve_attachments[i]);

			result->max_samples = std::max<VkSampleCountFlagBits>(result->max_samples, samples);
		}

		if (description.depthstencil_attachment)
			builder.setDepthStencilAttachmentReference(0, *description.depthstencil_attachment);

		result->render_pass = builder.build(context->getDevice());

		return reinterpret_cast<render::RenderPass>(result);
	}

	render::RenderPass Device::createRenderPass(
		render::SwapChain swap_chain,
		RenderPassLoadOp load_op,
		RenderPassStoreOp store_op,
		const RenderPassClearColor &clear_color
	)
	{
		assert(swap_chain);

		const SwapChain *vk_swap_chain = reinterpret_cast<const SwapChain *>(swap_chain);

		RenderPass *result = new RenderPass();

		result->num_attachments = 1;

		VkFormat vk_format = vk_swap_chain->surface_format.format;
		VkSampleCountFlagBits vk_samples = VK_SAMPLE_COUNT_1_BIT;
		VkAttachmentLoadOp vk_load_op = Utils::getLoadOp(load_op);
		VkAttachmentStoreOp vk_store_op = Utils::getStoreOp(store_op);

		result->attachment_formats[0] = vk_format;
		result->attachment_samples[0] = vk_samples;
		result->attachment_load_ops[0] = vk_load_op;
		result->attachment_store_ops[0] = vk_store_op;
		result->attachment_clear_values[0].color.float32[0] = clear_color.as_f32[0];
		result->attachment_clear_values[0].color.float32[1] = clear_color.as_f32[1];
		result->attachment_clear_values[0].color.float32[2] = clear_color.as_f32[2];
		result->attachment_clear_values[0].color.float32[3] = clear_color.as_f32[3];

		result->num_color_attachments = 1;
		result->max_samples = VK_SAMPLE_COUNT_1_BIT;

		RenderPassBuilder builder;
		result->render_pass = builder
			.addAttachment(vk_format, vk_samples, vk_load_op, vk_store_op, vk_load_op, vk_store_op, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
			.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
			.addColorAttachmentReference(0, 0)
			.build(context->getDevice());

		return reinterpret_cast<render::RenderPass>(result);
	}

	render::CommandBuffer Device::createCommandBuffer(
		CommandBufferType type
	)
	{
		CommandBuffer *result = new CommandBuffer();
		result->level = Utils::getCommandBufferLevel(type);

		// Allocate commandbuffer
		VkCommandBufferAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandPool = context->getCommandPool();
		info.level = result->level;
		info.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(context->getDevice(), &info, &result->command_buffer) != VK_SUCCESS)
		{
			std::cerr << "Device::createCommandBuffer(): can't allocate command buffer" << std::endl;
			destroyCommandBuffer(reinterpret_cast<render::CommandBuffer>(result));
			return nullptr;
		}

		// Create synchronization primitives
		VkSemaphoreCreateInfo semaphore_info = {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(context->getDevice(), &semaphore_info, nullptr, &result->rendering_finished_gpu) != VK_SUCCESS)
		{
			std::cerr << "Device::createCommandBuffer(): can't create 'rendering finished' semaphore" << std::endl;
			destroyCommandBuffer(reinterpret_cast<render::CommandBuffer>(result));
			return nullptr;
		}

		// fences
		VkFenceCreateInfo fence_info = {};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateFence(context->getDevice(), &fence_info, nullptr, &result->rendering_finished_cpu) != VK_SUCCESS)
		{
			std::cerr << "Device::createCommandBuffer(): can't create 'rendering finished' fence" << std::endl;
			destroyCommandBuffer(reinterpret_cast<render::CommandBuffer>(result));
			return nullptr;
		}

		return reinterpret_cast<render::CommandBuffer>(result);
	}

	render::UniformBuffer Device::createUniformBuffer(
		BufferType type,
		uint32_t size,
		const void *data
	)
	{
		assert(size != 0 && "Invalid size");

		UniformBuffer *result = new UniformBuffer();
		result->type = type;
		result->size = size;

		VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_UNKNOWN;

		if (type == BufferType::STATIC)
		{
			usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
		}
		else if (type == BufferType::DYNAMIC)
			memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		Utils::createBuffer(context, size, usage_flags, memory_usage,result->buffer, result->memory);

		if (data)
		{
			if (type == BufferType::STATIC)
				Utils::fillDeviceLocalBuffer(context, result->buffer, size, data);
			else if (type == BufferType::DYNAMIC)
				Utils::fillHostVisibleBuffer(context, result->memory, size, data);
		}

		return reinterpret_cast<render::UniformBuffer>(result);
	}

	render::Shader Device::createShaderFromSource(
		ShaderType type,
		uint32_t size,
		const char *data,
		const char *path
	)
	{
		assert(false && "Not implemented");
		return nullptr;
	}

	render::Shader Device::createShaderFromIL(
		ShaderType type,
		ShaderILType il_type,
		size_t size,
		const void *data
	)
	{
		assert(il_type == ShaderILType::SPIRV);
		assert(size > 0 && "Invalid shader IL size");
		assert(data != nullptr && "Invalid shader IL data");

		Shader *result = new Shader();
		result->type = type;
		result->module = Utils::createShaderModule(context, reinterpret_cast<const uint32_t *>(data), size);

		return reinterpret_cast<render::Shader>(result);
	}

	render::BindSet Device::createBindSet()
	{
		BindSet *result = new BindSet();
		memset(result, 0, sizeof(BindSet));

		return reinterpret_cast<render::BindSet>(result);
	}

	render::GraphicsPipeline Device::createGraphicsPipeline()
	{
		GraphicsPipeline *result = new GraphicsPipeline();
		memset(result, 0, sizeof(GraphicsPipeline));

		result->cull_mode = VK_CULL_MODE_BACK_BIT;
		result->primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		result->depth_compare_func = VK_COMPARE_OP_LESS_OR_EQUAL;

		return reinterpret_cast<render::GraphicsPipeline>(result);
	}

	render::BottomLevelAccelerationStructure Device::createBottomLevelAccelerationStructure(
		uint32_t num_geometries,
		const AccelerationStructureGeometry *geometries
	)
	{
		assert(num_geometries <= AccelerationStructure::MAX_GEOMETRIES);

		VkBuffer allocated_buffers[AccelerationStructure::MAX_GEOMETRIES * 3];
		VmaAllocation allocated_vram[AccelerationStructure::MAX_GEOMETRIES * 3];
		VkAccelerationStructureGeometryKHR vk_geometries[AccelerationStructure::MAX_GEOMETRIES];
		VkAccelerationStructureBuildRangeInfoKHR vk_ranges[AccelerationStructure::MAX_GEOMETRIES];
		uint32_t max_primitives[AccelerationStructure::MAX_GEOMETRIES];

		uint32_t num_allocations = 0;

		for (uint32_t i = 0; i < num_geometries; ++i)
		{
			const AccelerationStructureGeometry &geometry = geometries[i];

			VkDeviceSize vertex_buffer_size = Utils::getVertexSize(geometry.vertex_format) * geometry.num_vertices;
			VkDeviceSize index_buffer_size = Utils::getIndexSize(geometry.index_format) * geometry.num_indices;
			VkDeviceSize transform_buffer_size = sizeof(VkTransformMatrixKHR);

			VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

			// create vertex buffer
			VkDeviceOrHostAddressConstKHR vertex_buffer_address = {};
			VkBuffer vertex_buffer = VK_NULL_HANDLE;
			VmaAllocation vertex_buffer_memory = VK_NULL_HANDLE;

			Utils::createBuffer(context, vertex_buffer_size, usage_flags, memory_usage, vertex_buffer, vertex_buffer_memory);
			Utils::fillHostVisibleBuffer(context, vertex_buffer_memory, vertex_buffer_size, geometry.vertices);

			vertex_buffer_address.deviceAddress = Utils::getBufferDeviceAddress(context, vertex_buffer);

			allocated_buffers[num_allocations] = vertex_buffer;
			allocated_vram[num_allocations] = vertex_buffer_memory;

			num_allocations++;

			// create index buffer
			VkDeviceOrHostAddressConstKHR index_buffer_address = {};
			VkBuffer index_buffer = VK_NULL_HANDLE;
			VmaAllocation index_buffer_memory = VK_NULL_HANDLE;

			Utils::createBuffer(context, index_buffer_size, usage_flags, memory_usage, index_buffer, index_buffer_memory);
			Utils::fillHostVisibleBuffer(context, index_buffer_memory, index_buffer_size, geometry.indices);

			index_buffer_address.deviceAddress = Utils::getBufferDeviceAddress(context, index_buffer);

			allocated_buffers[num_allocations] = index_buffer;
			allocated_vram[num_allocations] = index_buffer_memory;

			num_allocations++;

			// create transform buffer
			VkTransformMatrixKHR transform = Utils::getTransformMatrix(geometry.transform);
			VkDeviceOrHostAddressConstKHR transform_buffer_address = {};
			VkBuffer transform_buffer = VK_NULL_HANDLE;
			VmaAllocation transform_buffer_memory = VK_NULL_HANDLE;

			Utils::createBuffer(context, transform_buffer_size, usage_flags, memory_usage, transform_buffer, transform_buffer_memory);
			Utils::fillHostVisibleBuffer(context, transform_buffer_memory, transform_buffer_size, &transform);

			allocated_buffers[num_allocations] = transform_buffer;
			allocated_vram[num_allocations] = transform_buffer_memory;

			transform_buffer_address.deviceAddress = Utils::getBufferDeviceAddress(context, transform_buffer);

			num_allocations++;

			VkAccelerationStructureGeometryKHR &vk_geometry = vk_geometries[i];
			vk_geometry = {};
			vk_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			vk_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			vk_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; // TODO: make it available to modify via API
			vk_geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			vk_geometry.geometry.triangles.vertexFormat = Utils::getFormat(geometry.vertex_format);
			vk_geometry.geometry.triangles.vertexData = vertex_buffer_address;
			vk_geometry.geometry.triangles.vertexStride = Utils::getVertexSize(geometry.vertex_format);
			vk_geometry.geometry.triangles.maxVertex = 3;
			vk_geometry.geometry.triangles.indexType = Utils::getIndexType(geometry.index_format);
			vk_geometry.geometry.triangles.indexData = index_buffer_address;
			vk_geometry.geometry.triangles.transformData = transform_buffer_address;

			VkAccelerationStructureBuildRangeInfoKHR &vk_range = vk_ranges[i];
			vk_range = {};
			vk_range.primitiveCount = geometry.num_indices / 3;
			vk_range.primitiveOffset = 0;
			vk_range.firstVertex = 0;
			vk_range.transformOffset = 0;

			max_primitives[i] = geometry.num_indices / 3;
		}

		AccelerationStructure *result = new AccelerationStructure();

		VkAccelerationStructureTypeKHR type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		VkBuildAccelerationStructureFlagBitsKHR build_flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; // TODO: pass as an argument

		if (!Utils::createAccelerationStructure(context, type, build_flags, num_geometries, vk_geometries, max_primitives, result))
		{
			for (size_t i = 0; i < num_allocations; ++i)
				vmaDestroyBuffer(context->getVRAMAllocator(), allocated_buffers[i], allocated_vram[i]);

			destroyBottomLevelAccelerationStructure(reinterpret_cast<render::BottomLevelAccelerationStructure>(result));
			return SCAPES_NULL_HANDLE;
		}

		Utils::buildAccelerationStructure(context, type, build_flags, num_geometries, vk_geometries, vk_ranges, result);

		for (size_t i = 0; i < num_allocations; ++i)
			vmaDestroyBuffer(context->getVRAMAllocator(), allocated_buffers[i], allocated_vram[i]);

		result->device_address = Utils::getBufferDeviceAddress(context, result->buffer);

		return reinterpret_cast<render::BottomLevelAccelerationStructure>(result);
	}

	render::TopLevelAccelerationStructure Device::createTopLevelAccelerationStructure(
		uint32_t num_instances,
		const AccelerationStructureInstance *instances
	)
	{
		std::vector<VkBuffer> allocated_buffers;
		std::vector<VmaAllocation> allocated_vram;
		std::vector<VkAccelerationStructureGeometryKHR> vk_geometries;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> vk_ranges;
		std::vector<uint32_t> max_primitives;

		for (uint32_t i = 0; i < num_instances; ++i)
		{
			const AccelerationStructureInstance &instance = instances[i];
			const AccelerationStructure *blas = reinterpret_cast<const AccelerationStructure *>(instance.blas);

			VkAccelerationStructureInstanceKHR vk_instance = {};
			vk_instance.instanceCustomIndex = 0; // TODO: make it available to modify via API
			vk_instance.instanceShaderBindingTableRecordOffset = 0; // TODO: make it available to modify via API
			vk_instance.mask = 0xFF;
			vk_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			vk_instance.accelerationStructureReference = blas->device_address;
			vk_instance.transform = Utils::getTransformMatrix(instance.transform);

			VkDeviceSize instance_size = sizeof(VkAccelerationStructureInstanceKHR);
			VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

			// create instance buffer
			VkDeviceOrHostAddressConstKHR instance_buffer_address = {};
			VkBuffer instance_buffer = VK_NULL_HANDLE;
			VmaAllocation instance_buffer_memory = VK_NULL_HANDLE;

			Utils::createBuffer(context, instance_size, usage_flags, memory_usage, instance_buffer, instance_buffer_memory);
			Utils::fillHostVisibleBuffer(context, instance_buffer_memory, instance_size, &vk_instance);

			instance_buffer_address.deviceAddress = Utils::getBufferDeviceAddress(context, instance_buffer);

			allocated_buffers.push_back(instance_buffer);
			allocated_vram.push_back(instance_buffer_memory);

			VkAccelerationStructureGeometryKHR vk_geometry = {};
			vk_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			vk_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
			vk_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; // TODO: make it available to modify via API
			vk_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
			vk_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
			vk_geometry.geometry.instances.data = instance_buffer_address;

			VkAccelerationStructureBuildRangeInfoKHR vk_range = {};
			vk_range.primitiveCount = 1;
			vk_range.primitiveOffset = 0;
			vk_range.firstVertex = 0;
			vk_range.transformOffset = 0;

			vk_ranges.push_back(vk_range);
			vk_geometries.push_back(vk_geometry);
			max_primitives.push_back(1);
		}

		AccelerationStructure *result = new AccelerationStructure();

		VkAccelerationStructureTypeKHR type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		VkBuildAccelerationStructureFlagBitsKHR build_flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; // TODO: pass as an argument

		if (!Utils::createAccelerationStructure(context, type, build_flags, num_instances, vk_geometries.data(), max_primitives.data(), result))
		{
			destroyTopLevelAccelerationStructure(reinterpret_cast<render::TopLevelAccelerationStructure>(result));
			return SCAPES_NULL_HANDLE;
		}

		Utils::buildAccelerationStructure(context, type, build_flags, num_instances, vk_geometries.data(), vk_ranges.data(), result);

		for (size_t i = 0; i < allocated_vram.size(); ++i)
			vmaDestroyBuffer(context->getVRAMAllocator(), allocated_buffers[i], allocated_vram[i]);

		result->device_address = Utils::getBufferDeviceAddress(context, result->buffer);

		return reinterpret_cast<render::TopLevelAccelerationStructure>(result);
	}

	render::RayTracePipeline Device::createRayTracePipeline(
		// ???
	)
	{
		RayTracePipeline *result = new RayTracePipeline();
		memset(result, 0, sizeof(RayTracePipeline));

		return reinterpret_cast<render::RayTracePipeline>(result);
	}

	render::SwapChain Device::createSwapChain(void *native_window)
	{
		assert(native_window != nullptr && "Invalid window");

		SwapChain *result = new SwapChain();

		// Create platform surface
		result->surface = Platform::createSurface(context, native_window);
		if (result->surface == VK_NULL_HANDLE)
		{
			std::cerr << "Device::createSwapChain(): can't create platform surface" << std::endl;
			destroySwapChain(reinterpret_cast<render::SwapChain>(result));
			return false;
		}

		// Fetch present queue family
		result->present_queue_family = Utils::getPresentQueueFamily(
			context->getPhysicalDevice(),
			result->surface,
			context->getGraphicsQueueFamily()
		);

		// Get present queue
		vkGetDeviceQueue(context->getDevice(), result->present_queue_family, 0, &result->present_queue);
		if (result->present_queue == VK_NULL_HANDLE)
		{
			std::cerr << "Device::createSwapChain(): can't get present queue from logical device" << std::endl;
			destroySwapChain(reinterpret_cast<render::SwapChain>(result));
			return false;
		}

		helpers::selectOptimalSwapChainSettings(context, result);
		helpers::createSwapChainObjects(context, result);

		return reinterpret_cast<render::SwapChain>(result);
	}

	void Device::destroyVertexBuffer(render::VertexBuffer vertex_buffer)
	{
		if (vertex_buffer == SCAPES_NULL_HANDLE)
			return;

		VertexBuffer *vk_vertex_buffer = reinterpret_cast<VertexBuffer *>(vertex_buffer);

		vmaDestroyBuffer(context->getVRAMAllocator(), vk_vertex_buffer->buffer, vk_vertex_buffer->memory);

		vk_vertex_buffer->buffer = VK_NULL_HANDLE;
		vk_vertex_buffer->memory = VK_NULL_HANDLE;

		delete vk_vertex_buffer;
		vk_vertex_buffer = nullptr;
	}

	void Device::destroyIndexBuffer(render::IndexBuffer index_buffer)
	{
		if (index_buffer == SCAPES_NULL_HANDLE)
			return;

		IndexBuffer *vk_index_buffer = reinterpret_cast<IndexBuffer *>(index_buffer);

		vmaDestroyBuffer(context->getVRAMAllocator(), vk_index_buffer->buffer, vk_index_buffer->memory);

		vk_index_buffer->buffer = VK_NULL_HANDLE;
		vk_index_buffer->memory = VK_NULL_HANDLE;

		delete vk_index_buffer;
		vk_index_buffer = nullptr;
	}

	void Device::destroyTexture(render::Texture texture)
	{
		if (texture == SCAPES_NULL_HANDLE)
			return;

		Texture *vk_texture = reinterpret_cast<Texture *>(texture);

		vmaDestroyImage(context->getVRAMAllocator(), vk_texture->image, vk_texture->memory);

		vk_texture->image = VK_NULL_HANDLE;
		vk_texture->memory = VK_NULL_HANDLE;
		vk_texture->format = VK_FORMAT_UNDEFINED;

		vkDestroySampler(context->getDevice(), vk_texture->sampler, nullptr);
		vk_texture->sampler = VK_NULL_HANDLE;

		delete vk_texture->image_view_cache;
		vk_texture->image_view_cache = nullptr;

		delete vk_texture;
		vk_texture = nullptr;
	}

	void Device::destroyStorageImage(render::StorageImage image)
	{
		if (image == SCAPES_NULL_HANDLE)
			return;

		Texture *vk_image = reinterpret_cast<Texture *>(image);

		vmaDestroyImage(context->getVRAMAllocator(), vk_image->image, vk_image->memory);

		vk_image->image = VK_NULL_HANDLE;
		vk_image->memory = VK_NULL_HANDLE;
		vk_image->format = VK_FORMAT_UNDEFINED;

		vkDestroySampler(context->getDevice(), vk_image->sampler, nullptr);
		vk_image->sampler = VK_NULL_HANDLE;

		delete vk_image->image_view_cache;
		vk_image->image_view_cache = nullptr;

		delete vk_image;
		vk_image = nullptr;
	}

	void Device::destroyFrameBuffer(render::FrameBuffer frame_buffer)
	{
		if (frame_buffer == SCAPES_NULL_HANDLE)
			return;

		FrameBuffer *vk_frame_buffer = reinterpret_cast<FrameBuffer *>(frame_buffer);

		for (uint8_t i = 0; i < vk_frame_buffer->num_attachments; ++i)
			vk_frame_buffer->attachment_views[i] = VK_NULL_HANDLE;

		vkDestroyFramebuffer(context->getDevice(), vk_frame_buffer->frame_buffer, nullptr);
		vk_frame_buffer->frame_buffer = VK_NULL_HANDLE;

		delete vk_frame_buffer;
		vk_frame_buffer = nullptr;
	}

	void Device::destroyRenderPass(render::RenderPass render_pass)
	{
		if (render_pass == SCAPES_NULL_HANDLE)
			return;

		RenderPass *vk_render_pass = reinterpret_cast<RenderPass *>(render_pass);

		vkDestroyRenderPass(context->getDevice(), vk_render_pass->render_pass, nullptr);
		vk_render_pass->render_pass = VK_NULL_HANDLE;

		delete vk_render_pass;
		vk_render_pass = nullptr;
	}

	void Device::destroyCommandBuffer(render::CommandBuffer command_buffer)
	{
		if (command_buffer == SCAPES_NULL_HANDLE)
			return;

		CommandBuffer *vk_command_buffer = reinterpret_cast<CommandBuffer *>(command_buffer);

		vkFreeCommandBuffers(context->getDevice(), context->getCommandPool(), 1, &vk_command_buffer->command_buffer);
		vk_command_buffer->command_buffer = VK_NULL_HANDLE;

		vkDestroySemaphore(context->getDevice(), vk_command_buffer->rendering_finished_gpu, nullptr);
		vk_command_buffer->rendering_finished_gpu = VK_NULL_HANDLE;

		vkDestroyFence(context->getDevice(), vk_command_buffer->rendering_finished_cpu, nullptr);
		vk_command_buffer->rendering_finished_cpu = VK_NULL_HANDLE;

		delete vk_command_buffer;
		vk_command_buffer = nullptr;
	}

	void Device::destroyUniformBuffer(render::UniformBuffer uniform_buffer)
	{
		if (uniform_buffer == SCAPES_NULL_HANDLE)
			return;

		UniformBuffer *vk_uniform_buffer = reinterpret_cast<UniformBuffer *>(uniform_buffer);

		vmaDestroyBuffer(context->getVRAMAllocator(), vk_uniform_buffer->buffer, vk_uniform_buffer->memory);

		vk_uniform_buffer->buffer = VK_NULL_HANDLE;
		vk_uniform_buffer->memory = VK_NULL_HANDLE;

		delete vk_uniform_buffer;
		vk_uniform_buffer = nullptr;
	}

	void Device::destroyShader(render::Shader shader)
	{
		if (shader == SCAPES_NULL_HANDLE)
			return;

		Shader *vk_shader = reinterpret_cast<Shader *>(shader);

		vkDestroyShaderModule(context->getDevice(), vk_shader->module, nullptr);
		vk_shader->module = VK_NULL_HANDLE;

		delete vk_shader;
		vk_shader = nullptr;
	}

	void Device::destroyBindSet(render::BindSet bind_set)
	{
		if (bind_set == SCAPES_NULL_HANDLE)
			return;

		BindSet *vk_bind_set = reinterpret_cast<BindSet *>(bind_set);

		for (uint32_t i = 0; i < BindSet::MAX_BINDINGS; ++i)
		{
			if (!vk_bind_set->binding_used[i])
				continue;

			VkDescriptorSetLayoutBinding &info = vk_bind_set->bindings[i];
			if (info.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				continue;

			BindSet::Data &data = vk_bind_set->binding_data[i];
		}

		if (vk_bind_set->set != VK_NULL_HANDLE)
			vkFreeDescriptorSets(context->getDevice(), context->getDescriptorPool(), 1, &vk_bind_set->set);

		delete vk_bind_set;
		vk_bind_set = nullptr;
	}

	void Device::destroyGraphicsPipeline(render::GraphicsPipeline pipeline)
	{
		if (pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(pipeline);

		delete vk_graphics_pipeline;
		vk_graphics_pipeline = nullptr;
	}

	void Device::destroyBottomLevelAccelerationStructure(render::BottomLevelAccelerationStructure acceleration_structure)
	{
		if (acceleration_structure == SCAPES_NULL_HANDLE)
			return;

		AccelerationStructure *vk_acceleration_structure = reinterpret_cast<AccelerationStructure *>(acceleration_structure);
		helpers::destroyAccelerationStructure(context, vk_acceleration_structure);
	}

	void Device::destroyTopLevelAccelerationStructure(render::TopLevelAccelerationStructure acceleration_structure)
	{
		if (acceleration_structure == SCAPES_NULL_HANDLE)
			return;

		AccelerationStructure *vk_acceleration_structure = reinterpret_cast<AccelerationStructure *>(acceleration_structure);
		helpers::destroyAccelerationStructure(context, vk_acceleration_structure);
	}

	void Device::destroyRayTracePipeline(render::RayTracePipeline pipeline)
	{
		if (pipeline == SCAPES_NULL_HANDLE)
			return;

		RayTracePipeline *vk_ray_trace_pipeline = reinterpret_cast<RayTracePipeline *>(pipeline);

		vmaDestroyBuffer(context->getVRAMAllocator(), vk_ray_trace_pipeline->sbt_buffer, vk_ray_trace_pipeline->sbt_memory);
		vk_ray_trace_pipeline->sbt_buffer = VK_NULL_HANDLE;
		vk_ray_trace_pipeline->sbt_memory = VK_NULL_HANDLE;

		delete vk_ray_trace_pipeline;
		vk_ray_trace_pipeline = nullptr;
	}

	void Device::destroySwapChain(render::SwapChain swap_chain)
	{
		if (swap_chain == SCAPES_NULL_HANDLE)
			return;

		SwapChain *vk_swap_chain = reinterpret_cast<SwapChain *>(swap_chain);

		helpers::destroySwapChainObjects(context, vk_swap_chain);

		vk_swap_chain->present_queue_family = 0xFFFF;
		vk_swap_chain->present_queue = VK_NULL_HANDLE;
		vk_swap_chain->present_mode = VK_PRESENT_MODE_FIFO_KHR;
		vk_swap_chain->sizes = {0, 0};

		vk_swap_chain->num_images = 0;
		vk_swap_chain->current_image = 0;
		vk_swap_chain->current_frame = 0;

		// Destroy platform surface
		Platform::destroySurface(context, vk_swap_chain->surface);
		vk_swap_chain->surface = nullptr;

		delete vk_swap_chain;
		vk_swap_chain = nullptr;
	}

	/*
	 */
	Multisample Device::getMaxSampleCount()
	{
		assert(context != nullptr && "Invalid context");

		VkSampleCountFlagBits samples = context->getMaxSampleCount();
		return Utils::getApiSamples(samples);
	}

	uint32_t Device::getNumSwapChainImages(render::SwapChain swap_chain)
	{
		assert(swap_chain != SCAPES_NULL_HANDLE && "Invalid swap chain");
		const SwapChain *vk_swap_chain = reinterpret_cast<const SwapChain *>(swap_chain);

		return vk_swap_chain->num_images;
	}

	void Device::setTextureSamplerWrapMode(render::Texture texture, SamplerWrapMode mode)
	{
		assert(texture != SCAPES_NULL_HANDLE && "Invalid texture");
		Texture *vk_texture = reinterpret_cast<Texture *>(texture);

		vkDestroySampler(context->getDevice(), vk_texture->sampler, nullptr);
		vk_texture->sampler = VK_NULL_HANDLE;

		VkSamplerAddressMode sampler_mode = Utils::getSamplerAddressMode(mode);
		vk_texture->sampler = Utils::createSampler(context, 0, vk_texture->num_mipmaps, sampler_mode, sampler_mode, sampler_mode);
	}

	void Device::setTextureSamplerDepthCompare(render::Texture texture, bool enabled, DepthCompareFunc func)
	{
		assert(texture != SCAPES_NULL_HANDLE && "Invalid texture");
		Texture *vk_texture = reinterpret_cast<Texture *>(texture);

		vkDestroySampler(context->getDevice(), vk_texture->sampler, nullptr);
		vk_texture->sampler = VK_NULL_HANDLE;

		VkSamplerAddressMode sampler_mode = Utils::getSamplerAddressMode(SamplerWrapMode::REPEAT);
		VkCompareOp compare_func = Utils::getDepthCompareFunc(func);
		vk_texture->sampler = Utils::createSampler(context, 0, vk_texture->num_mipmaps, sampler_mode, sampler_mode, sampler_mode, enabled, compare_func);
	}

	void Device::generateTexture2DMipmaps(render::Texture texture)
	{
		assert(texture != SCAPES_NULL_HANDLE && "Invalid texture");

		Texture *vk_texture = reinterpret_cast<Texture *>(texture);

		// prepare for transfer
		Utils::transitionImageLayout(
			context,
			vk_texture->image,
			vk_texture->format,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0,
			vk_texture->num_mipmaps
		);

		// generate 2D mipmaps with linear filter
		Utils::generateImage2DMipmaps(
			context,
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
			context,
			vk_texture->image,
			vk_texture->format,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			0,
			vk_texture->num_mipmaps
		);
	}

	/*
	 */
	void *Device::map(render::VertexBuffer vertex_buffer)
	{
		assert(vertex_buffer != SCAPES_NULL_HANDLE && "Invalid buffer");

		VertexBuffer *vk_vertex_buffer = reinterpret_cast<VertexBuffer *>(vertex_buffer);
		assert(vk_vertex_buffer->type == BufferType::DYNAMIC && "Mapped buffer must have BufferType::DYNAMIC type");

		void *result = nullptr;
		if (vmaMapMemory(context->getVRAMAllocator(), vk_vertex_buffer->memory, &result) != VK_SUCCESS)
		{
			// TODO: log error
		}

		return result;
	}

	void Device::unmap(render::VertexBuffer vertex_buffer)
	{
		assert(vertex_buffer != SCAPES_NULL_HANDLE && "Invalid buffer");

		VertexBuffer *vk_vertex_buffer = reinterpret_cast<VertexBuffer *>(vertex_buffer);
		assert(vk_vertex_buffer->type == BufferType::DYNAMIC && "Mapped buffer must have BufferType::DYNAMIC type");

		vmaUnmapMemory(context->getVRAMAllocator(), vk_vertex_buffer->memory);
	}

	void *Device::map(render::IndexBuffer index_buffer)
	{
		assert(index_buffer != SCAPES_NULL_HANDLE && "Invalid uniform buffer");

		IndexBuffer *vk_index_buffer = reinterpret_cast<IndexBuffer *>(index_buffer);
		assert(vk_index_buffer->type == BufferType::DYNAMIC && "Mapped buffer must have BufferType::DYNAMIC type");

		void *result = nullptr;
		if (vmaMapMemory(context->getVRAMAllocator(), vk_index_buffer->memory, &result) != VK_SUCCESS)
		{
			// TODO: log error
		}

		return result;
	}

	void Device::unmap(render::IndexBuffer index_buffer)
	{
		assert(index_buffer != SCAPES_NULL_HANDLE && "Invalid buffer");

		IndexBuffer *vk_index_buffer = reinterpret_cast<IndexBuffer *>(index_buffer);
		assert(vk_index_buffer->type == BufferType::DYNAMIC && "Mapped buffer must have BufferType::DYNAMIC type");

		vmaUnmapMemory(context->getVRAMAllocator(), vk_index_buffer->memory);
	}

	void *Device::map(render::UniformBuffer uniform_buffer)
	{
		assert(uniform_buffer != SCAPES_NULL_HANDLE && "Invalid uniform buffer");

		UniformBuffer *vk_uniform_buffer = reinterpret_cast<UniformBuffer *>(uniform_buffer);
		assert(vk_uniform_buffer->type == BufferType::DYNAMIC && "Mapped buffer must have BufferType::DYNAMIC type");

		void *result = nullptr;
		if (vmaMapMemory(context->getVRAMAllocator(), vk_uniform_buffer->memory, &result) != VK_SUCCESS)
		{
			// TODO: log error
		}

		return result;
	}

	void Device::unmap(render::UniformBuffer uniform_buffer)
	{
		assert(uniform_buffer != SCAPES_NULL_HANDLE && "Invalid buffer");

		UniformBuffer *vk_uniform_buffer = reinterpret_cast<UniformBuffer *>(uniform_buffer);
		assert(vk_uniform_buffer->type == BufferType::DYNAMIC && "Mapped buffer must have BufferType::DYNAMIC type");

		vmaUnmapMemory(context->getVRAMAllocator(), vk_uniform_buffer->memory);
	}

	/*
	 */
	void Device::flush(render::BindSet bind_set)
	{
		if (bind_set == SCAPES_NULL_HANDLE)
			return;

		BindSet *vk_bind_set = reinterpret_cast<BindSet *>(bind_set);

		VkWriteDescriptorSet writes[BindSet::MAX_BINDINGS];
		VkDescriptorImageInfo image_infos[BindSet::MAX_BINDINGS];
		VkDescriptorBufferInfo buffer_infos[BindSet::MAX_BINDINGS];
		VkWriteDescriptorSetAccelerationStructureKHR acceleration_structure_infos[BindSet::MAX_BINDINGS];

		uint32_t write_size = 0;
		uint32_t image_size = 0;
		uint32_t buffer_size = 0;
		uint32_t acceleration_structure_size = 0;

		VkDescriptorSetLayout new_layout = descriptor_set_layout_cache->fetch(vk_bind_set);
		helpers::updateBindSetLayout(context, vk_bind_set, new_layout);

		for (uint8_t i = 0; i < BindSet::MAX_BINDINGS; ++i)
		{
			if (!vk_bind_set->binding_used[i])
				continue;

			if (!vk_bind_set->binding_dirty[i])
				continue;

			VkDescriptorType descriptor_type = vk_bind_set->bindings[i].descriptorType;
			const BindSet::Data &data = vk_bind_set->binding_data[i];

			VkWriteDescriptorSet write_set = {};
			write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_set.dstSet = vk_bind_set->set;
			write_set.dstBinding = i;
			write_set.dstArrayElement = 0;

			switch (descriptor_type)
			{
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				{
					VkDescriptorImageInfo info = {};
					info.imageLayout = data.texture.layout;
					info.imageView = data.texture.view;
					info.sampler = data.texture.sampler;

					image_infos[image_size++] = info;
					write_set.pImageInfo = &image_infos[image_size - 1];
				}
				break;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				{
					VkDescriptorBufferInfo info = {};
					info.buffer = data.ubo.buffer;
					info.offset = data.ubo.offset;
					info.range = data.ubo.size;

					buffer_infos[buffer_size++] = info;
					write_set.pBufferInfo = &buffer_infos[buffer_size - 1];
				}
				break;
				case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
				{
					VkWriteDescriptorSetAccelerationStructureKHR info = {};
					info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
					info.accelerationStructureCount = 1;
					info.pAccelerationStructures = &data.tlas.acceleration_structure;

					acceleration_structure_infos[acceleration_structure_size++] = info;
					write_set.pNext = &acceleration_structure_infos[acceleration_structure_size - 1];
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

			writes[write_size++] = write_set;

			vk_bind_set->binding_dirty[i] = false;
		}

		if (write_size > 0)
			vkUpdateDescriptorSets(context->getDevice(), write_size, writes, 0, nullptr);
	}

	void Device::flush(render::GraphicsPipeline graphics_pipeline)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		for (uint32_t i = 0; i < vk_graphics_pipeline->num_bind_sets; ++i)
			flush(reinterpret_cast<render::BindSet>(vk_graphics_pipeline->bind_sets[i]));

		if (vk_graphics_pipeline->pipeline_layout == VK_NULL_HANDLE)
		{
			vk_graphics_pipeline->pipeline_layout = pipeline_layout_cache->fetch(vk_graphics_pipeline);
			vk_graphics_pipeline->pipeline = VK_NULL_HANDLE;
		}

		if (vk_graphics_pipeline->pipeline == VK_NULL_HANDLE)
			vk_graphics_pipeline->pipeline = pipeline_cache->fetch(vk_graphics_pipeline->pipeline_layout, vk_graphics_pipeline);
	}

	void Device::flush(render::RayTracePipeline raytrace_pipeline)
	{
		if (raytrace_pipeline == SCAPES_NULL_HANDLE)
			return;

		RayTracePipeline *vk_raytrace_pipeline = reinterpret_cast<RayTracePipeline *>(raytrace_pipeline);

		for (uint32_t i = 0; i < vk_raytrace_pipeline->num_bind_sets; ++i)
			flush(reinterpret_cast<render::BindSet>(vk_raytrace_pipeline->bind_sets[i]));

		if (vk_raytrace_pipeline->pipeline_layout == VK_NULL_HANDLE)
		{
			vk_raytrace_pipeline->pipeline_layout = pipeline_layout_cache->fetch(vk_raytrace_pipeline);
			vk_raytrace_pipeline->pipeline = VK_NULL_HANDLE;
		}

		if (vk_raytrace_pipeline->pipeline == VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(context->getVRAMAllocator(), vk_raytrace_pipeline->sbt_buffer, vk_raytrace_pipeline->sbt_memory);

			vk_raytrace_pipeline->sbt_memory = VK_NULL_HANDLE;
			vk_raytrace_pipeline->sbt_buffer = VK_NULL_HANDLE;

			vk_raytrace_pipeline->pipeline = pipeline_cache->fetch(vk_raytrace_pipeline->pipeline_layout, vk_raytrace_pipeline);
		}

		if (vk_raytrace_pipeline->sbt_memory == VK_NULL_HANDLE)
		{
			const uint32_t num_groups = vk_raytrace_pipeline->num_raygen_shaders + vk_raytrace_pipeline->num_miss_shaders + vk_raytrace_pipeline->num_hitgroup_shaders;
			const uint32_t sbt_handle_size_aligned = context->getSBTHandleSizeAligned();
			const uint32_t sbt_handle_size = context->getSBTHandleSize();

			const uint32_t sbt_src_raygen_size = vk_raytrace_pipeline->num_raygen_shaders * sbt_handle_size;
			const uint32_t sbt_dst_raygen_size = context->getSBTBaseSizeAligned(vk_raytrace_pipeline->num_raygen_shaders * sbt_handle_size_aligned);

			const uint32_t sbt_src_miss_size = vk_raytrace_pipeline->num_miss_shaders * sbt_handle_size;
			const uint32_t sbt_dst_miss_size = context->getSBTBaseSizeAligned(vk_raytrace_pipeline->num_miss_shaders * sbt_handle_size_aligned);

			const uint32_t sbt_src_hitgroup_size = vk_raytrace_pipeline->num_hitgroup_shaders * sbt_handle_size;
			const uint32_t sbt_dst_hitgroup_size = context->getSBTBaseSizeAligned(vk_raytrace_pipeline->num_hitgroup_shaders * sbt_handle_size_aligned);

			const uint32_t sbt_aligned_size = sbt_dst_raygen_size + sbt_dst_miss_size + sbt_dst_hitgroup_size;
			const uint32_t sbt_size = sbt_src_raygen_size + sbt_src_miss_size + sbt_src_hitgroup_size;

			// Create buffer
			Utils::createBuffer(
				context,
				sbt_aligned_size,
				VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				vk_raytrace_pipeline->sbt_buffer,
				vk_raytrace_pipeline->sbt_memory
			);

			uint8_t *shader_handle_storage = new uint8_t[sbt_size];
			if (vkGetRayTracingShaderGroupHandlesKHR(context->getDevice(), vk_raytrace_pipeline->pipeline, 0, num_groups, sbt_size, shader_handle_storage) != VK_SUCCESS)
			{
				// TODO: log error "Can't get shader group handles"
			}

			void *sbt_data = nullptr;
			if (vmaMapMemory(context->getVRAMAllocator(), vk_raytrace_pipeline->sbt_memory, &sbt_data) != VK_SUCCESS)
			{
				// TODO: log error "Can't map SBT buffer"
			}

			uint8_t *dst_raygen_begin = reinterpret_cast<uint8_t *>(sbt_data);
			uint8_t *dst_miss_begin = dst_raygen_begin + sbt_dst_raygen_size;
			uint8_t *dst_hitgroup_begin = dst_miss_begin + sbt_dst_miss_size;

			uint8_t *src_raygen_begin = shader_handle_storage;
			uint8_t *src_miss_begin = src_raygen_begin + sbt_src_raygen_size;
			uint8_t *src_hitgroup_begin = src_miss_begin + sbt_src_miss_size;

			uint8_t *dst = dst_raygen_begin;
			uint8_t *src = src_raygen_begin;

			for (uint8_t i = 0; i < vk_raytrace_pipeline->num_raygen_shaders; ++i)
			{
				memcpy(dst, src, sbt_handle_size);
				dst += sbt_handle_size_aligned;
				src += sbt_handle_size;
			}

			dst = dst_miss_begin;
			src = src_miss_begin;

			for (uint8_t i = 0; i < vk_raytrace_pipeline->num_miss_shaders; ++i)
			{
				memcpy(dst, src, sbt_handle_size);
				dst += sbt_handle_size_aligned;
				src += sbt_handle_size;
			}

			dst = dst_hitgroup_begin;
			src = src_hitgroup_begin;

			for (uint8_t i = 0; i < vk_raytrace_pipeline->num_hitgroup_shaders; ++i)
			{
				memcpy(dst, src, sbt_handle_size);
				dst += sbt_handle_size_aligned;
				src += sbt_handle_size;
			}

			vmaUnmapMemory(context->getVRAMAllocator(), vk_raytrace_pipeline->sbt_memory);

			VkDeviceAddress sbt_begin = Utils::getBufferDeviceAddress(context, vk_raytrace_pipeline->sbt_buffer);
			vk_raytrace_pipeline->sbt_raygen_begin = sbt_begin;
			vk_raytrace_pipeline->sbt_miss_begin = sbt_begin + sbt_dst_raygen_size;
			vk_raytrace_pipeline->sbt_hitgroup_begin = sbt_begin + sbt_dst_raygen_size + sbt_dst_miss_size;

			delete[] shader_handle_storage;
		}
	}

	/*
	 */
	bool Device::acquire(render::SwapChain swap_chain, uint32_t *new_image)
	{
		if (swap_chain == SCAPES_NULL_HANDLE)
			return false;

		SwapChain *vk_swap_chain = reinterpret_cast<SwapChain *>(swap_chain);

		vk_swap_chain->current_frame++;
		vk_swap_chain->current_frame %= vk_swap_chain->num_images;

		VkSemaphore sync = vk_swap_chain->image_available_gpu[vk_swap_chain->current_frame];

		VkResult result = vkAcquireNextImageKHR(
			context->getDevice(),
			vk_swap_chain->swap_chain,
			WAIT_NANOSECONDS,
			sync,
			VK_NULL_HANDLE,
			&vk_swap_chain->current_image
		);

		if (result == VK_TIMEOUT)
			return false;

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
			return false;

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			// TODO: log fatal
			// runtime_error("Can't acquire swap chain image");
			return false;
		}

		if (new_image)
			*new_image = vk_swap_chain->current_image;

		return true;
	}

	bool Device::present(render::SwapChain swap_chain, uint32_t num_wait_command_buffers, const render::CommandBuffer *wait_command_buffers)
	{
		SwapChain *vk_swap_chain = reinterpret_cast<SwapChain *>(swap_chain);

		std::vector<VkSemaphore> wait_semaphores(num_wait_command_buffers);

		if (num_wait_command_buffers != 0 && wait_command_buffers != nullptr)
		{
			for (uint32_t i = 0; i < num_wait_command_buffers; ++i)
			{
				CommandBuffer *vk_wait_command_buffer = reinterpret_cast<CommandBuffer *>(wait_command_buffers[i]);
				wait_semaphores[i] = vk_wait_command_buffer->rendering_finished_gpu;
			}
		}

		VkPresentInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.swapchainCount = 1;
		info.pSwapchains = &vk_swap_chain->swap_chain;
		info.pImageIndices = &vk_swap_chain->current_image;

		if (wait_semaphores.size())
		{
			info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());
			info.pWaitSemaphores = wait_semaphores.data();
		}

		VkResult result = vkQueuePresentKHR(vk_swap_chain->present_queue, &info);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			return false;

		if (result != VK_SUCCESS)
		{
			// TODO: log fatal
			// runtime_error("Can't present swap chain image");
			return false;
		}

		return true;
	}

	void Device::wait()
	{
		assert(context != nullptr && "Invalid context");

		vkDeviceWaitIdle(context->getDevice());
	}

	bool Device::wait(
		uint32_t num_wait_command_buffers,
		const render::CommandBuffer *wait_command_buffers
	)
	{
		if (num_wait_command_buffers == 0)
			return true;

		std::vector<VkFence> wait_fences(num_wait_command_buffers);

		for (uint32_t i = 0; i < num_wait_command_buffers; ++i)
		{
			CommandBuffer *vk_wait_command_buffer = reinterpret_cast<CommandBuffer *>(wait_command_buffers[i]);
			wait_fences[i] = vk_wait_command_buffer->rendering_finished_cpu;
		}

		VkResult result = vkWaitForFences(context->getDevice(), num_wait_command_buffers, wait_fences.data(), VK_TRUE, WAIT_NANOSECONDS);

		return result == VK_SUCCESS;
	}

	/*
	 */
	void Device::bindUniformBuffer(
		render::BindSet bind_set,
		uint32_t binding,
		render::UniformBuffer uniform_buffer
	)
	{
		assert(binding < BindSet::MAX_BINDINGS);

		if (bind_set == SCAPES_NULL_HANDLE)
			return;

		BindSet *vk_bind_set = reinterpret_cast<BindSet *>(bind_set);
		UniformBuffer *vk_uniform_buffer = reinterpret_cast<UniformBuffer *>(uniform_buffer);
		
		VkDescriptorSetLayoutBinding &info = vk_bind_set->bindings[binding];
		BindSet::Data &data = vk_bind_set->binding_data[binding];

		bool buffer_changed = (data.ubo.buffer != vk_uniform_buffer->buffer) || (data.ubo.size != vk_uniform_buffer->size);
		bool type_changed = (info.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

		vk_bind_set->binding_used[binding] = (vk_uniform_buffer != nullptr);
		vk_bind_set->binding_dirty[binding] = type_changed || buffer_changed;

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

	void Device::bindTexture(
		render::BindSet bind_set,
		uint32_t binding,
		render::Texture texture
	)
	{
		assert(binding < BindSet::MAX_BINDINGS);

		if (bind_set == SCAPES_NULL_HANDLE)
			return;

		BindSet *vk_bind_set = reinterpret_cast<BindSet *>(bind_set);
		Texture *vk_texture = reinterpret_cast<Texture *>(texture);

		uint32_t num_mipmaps = (vk_texture) ? vk_texture->num_mipmaps : 0;
		uint32_t num_layers = (vk_texture) ? vk_texture->num_layers : 0;

		bindTexture(bind_set, binding, texture, 0, num_mipmaps, 0, num_layers);
	}

	void Device::bindTexture(
		render::BindSet bind_set,
		uint32_t binding,
		render::StorageImage image
	)
	{
		assert(binding < BindSet::MAX_BINDINGS);

		if (bind_set == SCAPES_NULL_HANDLE)
			return;

		BindSet *vk_bind_set = reinterpret_cast<BindSet *>(bind_set);
		Texture *vk_image = reinterpret_cast<Texture *>(image);

		VkDescriptorSetLayoutBinding &info = vk_bind_set->bindings[binding];
		BindSet::Data &data = vk_bind_set->binding_data[binding];

		VkImageView view = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;

		if (vk_image)
		{
			view = vk_image->image_view_cache->fetch(vk_image);
			sampler = vk_image->sampler;
		}

		bool texture_changed = (data.texture.view != view) || (data.texture.sampler != sampler);
		bool type_changed = (info.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		vk_bind_set->binding_used[binding] = (vk_image != nullptr);
		vk_bind_set->binding_dirty[binding] = type_changed || texture_changed;

		data.texture.view = view;
		data.texture.sampler = sampler;
		data.texture.layout = VK_IMAGE_LAYOUT_GENERAL;

		info.binding = binding;
		info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		info.descriptorCount = 1;
		info.stageFlags = VK_SHADER_STAGE_ALL; // TODO: allow for different shader stages
		info.pImmutableSamplers = nullptr;
	}

	void Device::bindTexture(
		render::BindSet bind_set,
		uint32_t binding,
		render::Texture texture,
		uint32_t base_mip,
		uint32_t num_mipmaps,
		uint32_t base_layer,
		uint32_t num_layers
	)
	{
		assert(binding < BindSet::MAX_BINDINGS);

		if (bind_set == SCAPES_NULL_HANDLE)
			return;

		BindSet *vk_bind_set = reinterpret_cast<BindSet *>(bind_set);
		Texture *vk_texture = reinterpret_cast<Texture *>(texture);

		VkDescriptorSetLayoutBinding &info = vk_bind_set->bindings[binding];
		BindSet::Data &data = vk_bind_set->binding_data[binding];

		VkImageView view = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;

		if (vk_texture)
		{
			view = vk_texture->image_view_cache->fetch(vk_texture, base_mip, num_mipmaps, base_layer, num_layers);
			sampler = vk_texture->sampler;
		}

		bool texture_changed = (data.texture.view != view) || (data.texture.sampler != sampler);
		bool type_changed = (info.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		vk_bind_set->binding_used[binding] = (vk_texture != nullptr);
		vk_bind_set->binding_dirty[binding] = type_changed || texture_changed;

		data.texture.view = view;
		data.texture.sampler = sampler;
		data.texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		info.binding = binding;
		info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		info.descriptorCount = 1;
		info.stageFlags = VK_SHADER_STAGE_ALL; // TODO: allow for different shader stages
		info.pImmutableSamplers = nullptr;
	}

	void Device::bindTopLevelAccelerationStructure(
		render::BindSet bind_set,
		uint32_t binding,
		render::TopLevelAccelerationStructure tlas
	)
	{
		assert(binding < BindSet::MAX_BINDINGS);

		if (bind_set == SCAPES_NULL_HANDLE)
			return;

		BindSet *vk_bind_set = reinterpret_cast<BindSet *>(bind_set);
		AccelerationStructure *vk_tlas = reinterpret_cast<AccelerationStructure *>(tlas);

		VkDescriptorSetLayoutBinding &info = vk_bind_set->bindings[binding];
		BindSet::Data &data = vk_bind_set->binding_data[binding];

		VkAccelerationStructureKHR acceleration_structure = VK_NULL_HANDLE;

		if (vk_tlas)
		{
			acceleration_structure = vk_tlas->acceleration_structure;
		}

		bool tlas_changed = (data.tlas.acceleration_structure != acceleration_structure);
		bool type_changed = (info.descriptorType != VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

		vk_bind_set->binding_used[binding] = (vk_tlas != nullptr);
		vk_bind_set->binding_dirty[binding] = type_changed || tlas_changed;

		data.tlas.acceleration_structure = acceleration_structure;

		info.binding = binding;
		info.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		info.descriptorCount = 1;
		info.stageFlags = VK_SHADER_STAGE_ALL; // TODO: allow for different shader stages
		info.pImmutableSamplers = nullptr;
	}

	void Device::bindStorageImage(
		render::BindSet bind_set,
		uint32_t binding,
		render::StorageImage image
	)
	{
		assert(binding < BindSet::MAX_BINDINGS);

		if (bind_set == SCAPES_NULL_HANDLE)
			return;

		BindSet *vk_bind_set = reinterpret_cast<BindSet *>(bind_set);
		Texture *vk_image = reinterpret_cast<Texture *>(image);

		VkDescriptorSetLayoutBinding &info = vk_bind_set->bindings[binding];
		BindSet::Data &data = vk_bind_set->binding_data[binding];

		VkImageView view = VK_NULL_HANDLE;

		if (vk_image)
			view = vk_image->image_view_cache->fetch(vk_image);

		bool texture_changed = (data.texture.view != view);
		bool type_changed = (info.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		vk_bind_set->binding_used[binding] = (vk_image != nullptr);
		vk_bind_set->binding_dirty[binding] = type_changed || texture_changed;

		data.texture.view = view;
		data.texture.sampler = VK_NULL_HANDLE;
		data.texture.layout = VK_IMAGE_LAYOUT_GENERAL;

		info.binding = binding;
		info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		info.descriptorCount = 1;
		info.stageFlags = VK_SHADER_STAGE_ALL; // TODO: allow for different shader stages
		info.pImmutableSamplers = nullptr;
	}

	/*
	 */
	// raytrace pipeline state
	void Device::clearBindSets(
		render::RayTracePipeline raytrace_pipeline
	)
	{
		if (raytrace_pipeline == SCAPES_NULL_HANDLE)
			return;

		RayTracePipeline *vk_raytrace_pipeline = reinterpret_cast<RayTracePipeline *>(raytrace_pipeline);

		for (uint32_t i = 0; i < RayTracePipeline::MAX_BIND_SETS; ++i)
			vk_raytrace_pipeline->bind_sets[i] = nullptr;

		vk_raytrace_pipeline->num_bind_sets = 0;

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_raytrace_pipeline->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::setBindSet(render::RayTracePipeline raytrace_pipeline, uint8_t binding, render::BindSet bind_set)
	{
		assert(binding < RayTracePipeline::MAX_BIND_SETS);

		if (raytrace_pipeline == SCAPES_NULL_HANDLE)
			return;

		RayTracePipeline *vk_raytrace_pipeline = reinterpret_cast<RayTracePipeline *>(raytrace_pipeline);
		BindSet *vk_bind_set = reinterpret_cast<BindSet *>(bind_set);

		vk_raytrace_pipeline->bind_sets[binding] = vk_bind_set;
		vk_raytrace_pipeline->num_bind_sets = std::max<uint32_t>(vk_raytrace_pipeline->num_bind_sets, binding + 1);

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_raytrace_pipeline->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::clearRaygenShaders(render::RayTracePipeline raytrace_pipeline)
	{
		if (raytrace_pipeline == SCAPES_NULL_HANDLE)
			return;

		RayTracePipeline *vk_raytrace_pipeline = reinterpret_cast<RayTracePipeline *>(raytrace_pipeline);

		for (uint32_t i = 0; i < RayTracePipeline::MAX_RAYGEN_SHADERS; ++i)
			vk_raytrace_pipeline->raygen_shaders[i] = VK_NULL_HANDLE;

		vk_raytrace_pipeline->num_raygen_shaders = 0;
		vk_raytrace_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::addRaygenShader(render::RayTracePipeline raytrace_pipeline, render::Shader shader)
	{
		if (raytrace_pipeline == SCAPES_NULL_HANDLE)
			return;

		if (shader == SCAPES_NULL_HANDLE)
			return;

		RayTracePipeline *vk_raytrace_pipeline = reinterpret_cast<RayTracePipeline *>(raytrace_pipeline);
		assert(vk_raytrace_pipeline->num_raygen_shaders <= RayTracePipeline::MAX_RAYGEN_SHADERS);

		Shader *vk_shader = reinterpret_cast<Shader *>(shader);
		assert(vk_shader->type == render::ShaderType::RAY_GENERATION);

		vk_raytrace_pipeline->raygen_shaders[vk_raytrace_pipeline->num_raygen_shaders] = vk_shader->module;
		vk_raytrace_pipeline->num_raygen_shaders++;
		vk_raytrace_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::clearHitGroupShaders(render::RayTracePipeline raytrace_pipeline)
	{
		if (raytrace_pipeline == SCAPES_NULL_HANDLE)
			return;

		RayTracePipeline *vk_raytrace_pipeline = reinterpret_cast<RayTracePipeline *>(raytrace_pipeline);

		for (uint32_t i = 0; i < RayTracePipeline::MAX_HITGROUP_SHADERS; ++i)
		{
			vk_raytrace_pipeline->intersection_shaders[i] = VK_NULL_HANDLE;
			vk_raytrace_pipeline->anyhit_shaders[i] = VK_NULL_HANDLE;
			vk_raytrace_pipeline->closesthit_shaders[i] = VK_NULL_HANDLE;
		}

		vk_raytrace_pipeline->num_hitgroup_shaders = 0;
		vk_raytrace_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::addHitGroupShader(
		render::RayTracePipeline raytrace_pipeline,
		render::Shader intersection_shader,
		render::Shader anyhit_shader,
		render::Shader closesthit_shader
	)
	{
		if (raytrace_pipeline == SCAPES_NULL_HANDLE)
			return;

		if (intersection_shader == SCAPES_NULL_HANDLE && anyhit_shader == SCAPES_NULL_HANDLE && closesthit_shader == SCAPES_NULL_HANDLE)
			return;

		RayTracePipeline *vk_raytrace_pipeline = reinterpret_cast<RayTracePipeline *>(raytrace_pipeline);
		assert(vk_raytrace_pipeline->num_hitgroup_shaders <= RayTracePipeline::MAX_HITGROUP_SHADERS);

		Shader *vk_intersection_shader = reinterpret_cast<Shader *>(intersection_shader);
		Shader *vk_anyhit_shader = reinterpret_cast<Shader *>(anyhit_shader);
		Shader *vk_closesthit_shader = reinterpret_cast<Shader *>(closesthit_shader);

		assert(vk_intersection_shader == nullptr || vk_intersection_shader->type == render::ShaderType::INTERSECTION);
		assert(vk_anyhit_shader == nullptr || vk_anyhit_shader->type == render::ShaderType::ANY_HIT);
		assert(vk_closesthit_shader == nullptr || vk_closesthit_shader->type == render::ShaderType::CLOSEST_HIT);

		VkShaderModule intersection_module = (vk_intersection_shader) ? vk_intersection_shader->module : VK_NULL_HANDLE;
		VkShaderModule anyhit_module = (vk_anyhit_shader) ? vk_anyhit_shader->module : VK_NULL_HANDLE;
		VkShaderModule closesthit_module = (vk_closesthit_shader) ? vk_closesthit_shader->module : VK_NULL_HANDLE;

		vk_raytrace_pipeline->intersection_shaders[vk_raytrace_pipeline->num_hitgroup_shaders] = intersection_module;
		vk_raytrace_pipeline->anyhit_shaders[vk_raytrace_pipeline->num_hitgroup_shaders] = anyhit_module;
		vk_raytrace_pipeline->closesthit_shaders[vk_raytrace_pipeline->num_hitgroup_shaders] = closesthit_module;
		vk_raytrace_pipeline->num_hitgroup_shaders++;
		vk_raytrace_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::clearMissShaders(render::RayTracePipeline raytrace_pipeline)
	{
		if (raytrace_pipeline == SCAPES_NULL_HANDLE)
			return;

		RayTracePipeline *vk_raytrace_pipeline = reinterpret_cast<RayTracePipeline *>(raytrace_pipeline);

		for (uint32_t i = 0; i < RayTracePipeline::MAX_MISS_SHADERS; ++i)
			vk_raytrace_pipeline->miss_shaders[i] = VK_NULL_HANDLE;

		vk_raytrace_pipeline->num_miss_shaders = 0;
		vk_raytrace_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::addMissShader(render::RayTracePipeline raytrace_pipeline, render::Shader shader)
	{
		if (raytrace_pipeline == SCAPES_NULL_HANDLE)
			return;

		if (shader == SCAPES_NULL_HANDLE)
			return;

		RayTracePipeline *vk_raytrace_pipeline = reinterpret_cast<RayTracePipeline *>(raytrace_pipeline);
		assert(vk_raytrace_pipeline->num_miss_shaders <= RayTracePipeline::MAX_MISS_SHADERS);

		Shader *vk_shader = reinterpret_cast<Shader *>(shader);
		assert(vk_shader->type == render::ShaderType::MISS);

		vk_raytrace_pipeline->miss_shaders[vk_raytrace_pipeline->num_miss_shaders] = vk_shader->module;
		vk_raytrace_pipeline->num_miss_shaders++;
		vk_raytrace_pipeline->pipeline = VK_NULL_HANDLE;
	}

	/*
	 */
	void Device::clearPushConstants(render::GraphicsPipeline graphics_pipeline)
	{
		if (graphics_pipeline == nullptr)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		vk_graphics_pipeline->push_constants_size = 0;
		memset(vk_graphics_pipeline->push_constants, 0, GraphicsPipeline::MAX_PUSH_CONSTANT_SIZE);

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_graphics_pipeline->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::setPushConstants(render::GraphicsPipeline graphics_pipeline, uint8_t size, const void *data)
	{
		assert(size <= GraphicsPipeline::MAX_PUSH_CONSTANT_SIZE);

		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		vk_graphics_pipeline->push_constants_size = size;
		memcpy(vk_graphics_pipeline->push_constants, data, size);

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_graphics_pipeline->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::clearBindSets(render::GraphicsPipeline graphics_pipeline)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		for (uint32_t i = 0; i < GraphicsPipeline::MAX_BIND_SETS; ++i)
			vk_graphics_pipeline->bind_sets[i] = nullptr;

		vk_graphics_pipeline->num_bind_sets = 0;

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_graphics_pipeline->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::setBindSet(render::GraphicsPipeline graphics_pipeline, uint8_t binding, render::BindSet bind_set)
	{
		assert(binding < GraphicsPipeline::MAX_BIND_SETS);

		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);
		BindSet *vk_bind_set = reinterpret_cast<BindSet *>(bind_set);

		vk_graphics_pipeline->bind_sets[binding] = vk_bind_set;
		vk_graphics_pipeline->num_bind_sets = std::max<uint32_t>(vk_graphics_pipeline->num_bind_sets, binding + 1);

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_graphics_pipeline->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::clearShaders(render::GraphicsPipeline graphics_pipeline)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		for (uint32_t i = 0; i < GraphicsPipeline::MAX_SHADERS; ++i)
			vk_graphics_pipeline->shaders[i] = VK_NULL_HANDLE;

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_graphics_pipeline->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::setShader(render::GraphicsPipeline graphics_pipeline, ShaderType type, render::Shader shader)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);
		Shader *vk_shader = reinterpret_cast<Shader *>(shader);

		vk_graphics_pipeline->shaders[static_cast<int>(type)] = (vk_shader) ? vk_shader->module : VK_NULL_HANDLE;

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_graphics_pipeline->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::clearVertexStreams(render::GraphicsPipeline graphics_pipeline)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		for (uint32_t i = 0; i < GraphicsPipeline::MAX_VERTEX_STREAMS; ++i)
			vk_graphics_pipeline->vertex_streams[i] = nullptr;

		vk_graphics_pipeline->num_vertex_streams = 0;

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_graphics_pipeline->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::setVertexStream(render::GraphicsPipeline graphics_pipeline, uint8_t binding, render::VertexBuffer vertex_buffer)
	{
		assert(binding < GraphicsPipeline::MAX_VERTEX_STREAMS);

		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);
		VertexBuffer *vk_vertex_buffer = reinterpret_cast<VertexBuffer *>(vertex_buffer);

		vk_graphics_pipeline->vertex_streams[binding] = vk_vertex_buffer;
		vk_graphics_pipeline->num_vertex_streams = std::max<uint32_t>(vk_graphics_pipeline->num_vertex_streams, binding + 1);

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_graphics_pipeline->pipeline_layout = VK_NULL_HANDLE;
	}

	/*
	 */
	void Device::setViewport(render::GraphicsPipeline graphics_pipeline, int32_t x, int32_t y, uint32_t width, uint32_t height)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		vk_graphics_pipeline->viewport.x = static_cast<float>(x);
		vk_graphics_pipeline->viewport.y = static_cast<float>(y);
		vk_graphics_pipeline->viewport.width = static_cast<float>(width);
		vk_graphics_pipeline->viewport.height = static_cast<float>(height);
		vk_graphics_pipeline->viewport.minDepth = 0.0f;
		vk_graphics_pipeline->viewport.maxDepth = 1.0f;

		vk_graphics_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::setScissor(render::GraphicsPipeline graphics_pipeline, int32_t x, int32_t y, uint32_t width, uint32_t height)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		vk_graphics_pipeline->scissor.offset = { x, y };
		vk_graphics_pipeline->scissor.extent = { width, height };

		vk_graphics_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::setPrimitiveType(render::GraphicsPipeline graphics_pipeline, RenderPrimitiveType type)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		vk_graphics_pipeline->primitive_topology = Utils::getPrimitiveTopology(type);

		vk_graphics_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::setCullMode(render::GraphicsPipeline graphics_pipeline, CullMode mode)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		vk_graphics_pipeline->cull_mode = Utils::getCullMode(mode);

		vk_graphics_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::setDepthTest(render::GraphicsPipeline graphics_pipeline, bool enabled)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		vk_graphics_pipeline->depth_test = enabled;

		vk_graphics_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::setDepthWrite(render::GraphicsPipeline graphics_pipeline, bool enabled)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		vk_graphics_pipeline->depth_write = enabled;

		vk_graphics_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::setDepthCompareFunc(render::GraphicsPipeline graphics_pipeline, DepthCompareFunc func)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		vk_graphics_pipeline->depth_compare_func = Utils::getDepthCompareFunc(func);

		vk_graphics_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::setBlending(render::GraphicsPipeline graphics_pipeline, bool enabled)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		vk_graphics_pipeline->blending = enabled;

		vk_graphics_pipeline->pipeline = VK_NULL_HANDLE;
	}

	void Device::setBlendFactors(render::GraphicsPipeline graphics_pipeline, BlendFactor src_factor, BlendFactor dst_factor)
	{
		if (graphics_pipeline == SCAPES_NULL_HANDLE)
			return;

		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);

		vk_graphics_pipeline->blend_src_factor = Utils::getBlendFactor(src_factor);
		vk_graphics_pipeline->blend_dst_factor = Utils::getBlendFactor(dst_factor);

		vk_graphics_pipeline->pipeline = VK_NULL_HANDLE;
	}

	/*
	 */
	bool Device::resetCommandBuffer(render::CommandBuffer command_buffer)
	{
		if (command_buffer == SCAPES_NULL_HANDLE)
			return false;

		CommandBuffer *vk_command_buffer = reinterpret_cast<CommandBuffer *>(command_buffer);
		if (vkResetCommandBuffer(vk_command_buffer->command_buffer, 0) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Device::beginCommandBuffer(render::CommandBuffer command_buffer)
	{
		if (command_buffer == SCAPES_NULL_HANDLE)
			return false;

		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		CommandBuffer *vk_command_buffer = reinterpret_cast<CommandBuffer *>(command_buffer);
		if (vkBeginCommandBuffer(vk_command_buffer->command_buffer, &info) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Device::endCommandBuffer(render::CommandBuffer command_buffer)
	{
		if (command_buffer == SCAPES_NULL_HANDLE)
			return false;

		CommandBuffer *vk_command_buffer = reinterpret_cast<CommandBuffer *>(command_buffer);
		if (vkEndCommandBuffer(vk_command_buffer->command_buffer) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Device::submit(render::CommandBuffer command_buffer)
	{
		if (command_buffer == SCAPES_NULL_HANDLE)
			return false;
		
		CommandBuffer *vk_command_buffer = reinterpret_cast<CommandBuffer *>(command_buffer);

		VkSubmitInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &vk_command_buffer->command_buffer;

		vkResetFences(context->getDevice(), 1, &vk_command_buffer->rendering_finished_cpu);
		if (vkQueueSubmit(context->getGraphicsQueue(), 1, &info, vk_command_buffer->rendering_finished_cpu) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Device::submitSyncked(render::CommandBuffer command_buffer, render::SwapChain wait_swap_chain)
	{
		if (command_buffer == SCAPES_NULL_HANDLE)
			return false;
		
		CommandBuffer *vk_command_buffer = reinterpret_cast<CommandBuffer *>(command_buffer);

		VkSubmitInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &vk_command_buffer->command_buffer;
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &vk_command_buffer->rendering_finished_gpu;

		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		if (wait_swap_chain != nullptr)
		{
			SwapChain *vk_wait_swap_chain = reinterpret_cast<SwapChain *>(wait_swap_chain);
			uint32_t current_frame = vk_wait_swap_chain->current_frame;

			info.waitSemaphoreCount = 1;
			info.pWaitSemaphores = &vk_wait_swap_chain->image_available_gpu[current_frame];
			info.pWaitDstStageMask = &wait_stage;
		}

		vkResetFences(context->getDevice(), 1, &vk_command_buffer->rendering_finished_cpu);
		if (vkQueueSubmit(context->getGraphicsQueue(), 1, &info, vk_command_buffer->rendering_finished_cpu) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Device::submitSyncked(render::CommandBuffer command_buffer, uint32_t num_wait_command_buffers, const render::CommandBuffer *wait_command_buffers)
	{
		if (command_buffer == SCAPES_NULL_HANDLE)
			return false;
		
		CommandBuffer *vk_command_buffer = reinterpret_cast<CommandBuffer *>(command_buffer);

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
				CommandBuffer *vk_wait_command_buffer = reinterpret_cast<CommandBuffer *>(wait_command_buffers[i]);
				wait_semaphores[i] = vk_wait_command_buffer->rendering_finished_gpu;
				wait_stages[i] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			}

			info.waitSemaphoreCount = num_wait_command_buffers;
			info.pWaitSemaphores = wait_semaphores.data();
			info.pWaitDstStageMask = wait_stages.data();
		}

		vkResetFences(context->getDevice(), 1, &vk_command_buffer->rendering_finished_cpu);
		if (vkQueueSubmit(context->getGraphicsQueue(), 1, &info, vk_command_buffer->rendering_finished_cpu) != VK_SUCCESS)
			return false;

		return true;
	}

	void Device::beginRenderPass(render::CommandBuffer command_buffer, render::RenderPass render_pass, render::FrameBuffer frame_buffer)
	{
		assert(command_buffer != SCAPES_NULL_HANDLE);
		assert(render_pass != SCAPES_NULL_HANDLE);
		assert(frame_buffer != SCAPES_NULL_HANDLE);

		CommandBuffer *vk_command_buffer = reinterpret_cast<CommandBuffer *>(command_buffer);
		RenderPass *vk_render_pass = reinterpret_cast<RenderPass *>(render_pass);
		FrameBuffer *vk_frame_buffer = reinterpret_cast<FrameBuffer *>(frame_buffer);

		// lazily create framebuffer
		if (vk_frame_buffer->frame_buffer == VK_NULL_HANDLE)
		{
			VkFramebufferCreateInfo framebuffer_info = {};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = vk_render_pass->render_pass;
			framebuffer_info.attachmentCount = vk_frame_buffer->num_attachments;
			framebuffer_info.pAttachments = vk_frame_buffer->attachment_views;
			framebuffer_info.width = vk_frame_buffer->sizes.width;
			framebuffer_info.height = vk_frame_buffer->sizes.height;
			framebuffer_info.layers = vk_frame_buffer->num_layers;

			if (vkCreateFramebuffer(context->getDevice(), &framebuffer_info, nullptr, &vk_frame_buffer->frame_buffer) != VK_SUCCESS)
			{
				vk_frame_buffer->frame_buffer = VK_NULL_HANDLE;
				// TODO: log error
				return;
			}
		}

		VkRect2D render_area = {};
		render_area.extent = vk_frame_buffer->sizes;
		render_area.offset = {0, 0};

		vk_command_buffer->render_pass = vk_render_pass->render_pass;
		vk_command_buffer->max_samples = vk_render_pass->max_samples;
		vk_command_buffer->num_color_attachments = vk_render_pass->num_color_attachments;

		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = vk_render_pass->render_pass;
		render_pass_info.framebuffer = vk_frame_buffer->frame_buffer;
		render_pass_info.renderArea = render_area;
		render_pass_info.clearValueCount = vk_frame_buffer->num_attachments;
		render_pass_info.pClearValues = vk_render_pass->attachment_clear_values;

		vkCmdBeginRenderPass(vk_command_buffer->command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void Device::beginRenderPass(render::CommandBuffer command_buffer, render::RenderPass render_pass, render::SwapChain swap_chain)
	{
		assert(command_buffer != SCAPES_NULL_HANDLE);
		assert(render_pass != SCAPES_NULL_HANDLE);
		assert(swap_chain != SCAPES_NULL_HANDLE);

		CommandBuffer *vk_command_buffer = reinterpret_cast<CommandBuffer *>(command_buffer);
		RenderPass *vk_render_pass = reinterpret_cast<RenderPass *>(render_pass);
		SwapChain *vk_swap_chain = reinterpret_cast<SwapChain *>(swap_chain);

		VkFramebuffer frame_buffer = vk_swap_chain->frame_buffers[vk_swap_chain->current_image];

		VkRect2D render_area = {};
		render_area.extent = vk_swap_chain->sizes;
		render_area.offset = {0, 0};

		vk_command_buffer->render_pass = vk_render_pass->render_pass;
		vk_command_buffer->max_samples = vk_render_pass->max_samples;
		vk_command_buffer->num_color_attachments = vk_render_pass->num_color_attachments;

		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = vk_render_pass->render_pass;
		render_pass_info.framebuffer = frame_buffer;
		render_pass_info.renderArea = render_area;
		render_pass_info.clearValueCount = vk_render_pass->num_attachments;
		render_pass_info.pClearValues = vk_render_pass->attachment_clear_values;

		vkCmdBeginRenderPass(vk_command_buffer->command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void Device::endRenderPass(render::CommandBuffer command_buffer)
	{
		if (command_buffer == SCAPES_NULL_HANDLE)
			return;

		CommandBuffer *vk_command_buffer = reinterpret_cast<CommandBuffer *>(command_buffer);
		vkCmdEndRenderPass(vk_command_buffer->command_buffer);

		vk_command_buffer->render_pass = VK_NULL_HANDLE;
	}

	void Device::drawIndexedPrimitiveInstanced(
		render::CommandBuffer command_buffer,
		render::GraphicsPipeline graphics_pipeline,
		render::IndexBuffer index_buffer,
		uint32_t num_indices,
		uint32_t base_index,
		int32_t base_vertex,
		uint32_t num_instances,
		uint32_t base_instance
	)
	{
		SCAPES_PROFILER();

		if (command_buffer == SCAPES_NULL_HANDLE)
			return;

		CommandBuffer *vk_command_buffer = reinterpret_cast<CommandBuffer *>(command_buffer);
		GraphicsPipeline *vk_graphics_pipeline = reinterpret_cast<GraphicsPipeline *>(graphics_pipeline);
		IndexBuffer *vk_index_buffer = reinterpret_cast<IndexBuffer *>(index_buffer);

		assert(vk_command_buffer->render_pass != VK_NULL_HANDLE);

		vk_graphics_pipeline->render_pass = vk_command_buffer->render_pass;
		vk_graphics_pipeline->num_color_attachments = vk_command_buffer->num_color_attachments;
		vk_graphics_pipeline->max_samples = vk_command_buffer->max_samples;

		flush(graphics_pipeline);

		SCAPES_PROFILER_N("Actual draw call");

		VkPipeline pipeline = vk_graphics_pipeline->pipeline;
		VkPipelineLayout pipeline_layout = vk_graphics_pipeline->pipeline_layout;
		VkViewport viewport = vk_graphics_pipeline->viewport;
		VkRect2D scissor = vk_graphics_pipeline->scissor;

		vkCmdSetViewport(vk_command_buffer->command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(vk_command_buffer->command_buffer, 0, 1, &scissor);

		vkCmdBindPipeline(vk_command_buffer->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		if (vk_graphics_pipeline->push_constants_size > 0)
			vkCmdPushConstants(vk_command_buffer->command_buffer, pipeline_layout, VK_SHADER_STAGE_ALL, 0, vk_graphics_pipeline->push_constants_size, vk_graphics_pipeline->push_constants);

		if (vk_graphics_pipeline->num_bind_sets > 0)
		{
			VkDescriptorSet sets[GraphicsPipeline::MAX_BIND_SETS];
			for (uint32_t i = 0; i < vk_graphics_pipeline->num_bind_sets; ++i)
				sets[i] = vk_graphics_pipeline->bind_sets[i]->set;

			vkCmdBindDescriptorSets(vk_command_buffer->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, vk_graphics_pipeline->num_bind_sets, sets, 0, nullptr);
		}

		if (vk_graphics_pipeline->num_vertex_streams > 0)
		{
			VkBuffer vertex_buffers[GraphicsPipeline::MAX_VERTEX_STREAMS];
			VkDeviceSize offsets[GraphicsPipeline::MAX_VERTEX_STREAMS];

			for (uint32_t i = 0; i < vk_graphics_pipeline->num_vertex_streams; ++i)
			{
				vertex_buffers[i] = vk_graphics_pipeline->vertex_streams[i]->buffer;
				offsets[i] = 0;
			}

			vkCmdBindVertexBuffers(vk_command_buffer->command_buffer, 0, vk_graphics_pipeline->num_vertex_streams, vertex_buffers, offsets);
		}

		if (vk_index_buffer)
			vkCmdBindIndexBuffer(vk_command_buffer->command_buffer, vk_index_buffer->buffer, 0, vk_index_buffer->index_type);

		vkCmdDrawIndexed(vk_command_buffer->command_buffer, num_indices, num_instances, base_index, base_vertex, base_instance);
	}

	void Device::traceRays(
		render::CommandBuffer command_buffer,
		render::RayTracePipeline raytrace_pipeline,
		uint32_t width,
		uint32_t height,
		uint32_t depth,
		uint32_t raygen_shader_index
	)
	{
		SCAPES_PROFILER();

		if (command_buffer == SCAPES_NULL_HANDLE)
			return;

		CommandBuffer *vk_command_buffer = reinterpret_cast<CommandBuffer *>(command_buffer);
		RayTracePipeline *vk_raytrace_pipeline = reinterpret_cast<RayTracePipeline *>(raytrace_pipeline);

		flush(raytrace_pipeline);

		VkPipeline pipeline = vk_raytrace_pipeline->pipeline;
		VkPipelineLayout pipeline_layout = vk_raytrace_pipeline->pipeline_layout;

		uint32_t sbt_handle_size = context->getSBTHandleSizeAligned();

		VkStridedDeviceAddressRegionKHR raygen_sbt_buffer_entry{};
		raygen_sbt_buffer_entry.deviceAddress = vk_raytrace_pipeline->sbt_raygen_begin + raygen_shader_index * sbt_handle_size;
		raygen_sbt_buffer_entry.stride = sbt_handle_size;
		raygen_sbt_buffer_entry.size = sbt_handle_size;

		VkStridedDeviceAddressRegionKHR miss_sbt_buffer_entry{};
		miss_sbt_buffer_entry.deviceAddress = vk_raytrace_pipeline->sbt_miss_begin;
		miss_sbt_buffer_entry.stride = sbt_handle_size;
		miss_sbt_buffer_entry.size = sbt_handle_size;

		VkStridedDeviceAddressRegionKHR hitgroup_sbt_buffer_entry{};
		hitgroup_sbt_buffer_entry.deviceAddress = vk_raytrace_pipeline->sbt_hitgroup_begin;
		hitgroup_sbt_buffer_entry.stride = sbt_handle_size;
		hitgroup_sbt_buffer_entry.size = sbt_handle_size;

		VkStridedDeviceAddressRegionKHR callable_sbt_buffer_entry{};

		vkCmdBindPipeline(vk_command_buffer->command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);

		if (vk_raytrace_pipeline->num_bind_sets > 0)
		{
			VkDescriptorSet sets[RayTracePipeline::MAX_BIND_SETS];
			for (uint32_t i = 0; i < vk_raytrace_pipeline->num_bind_sets; ++i)
				sets[i] = vk_raytrace_pipeline->bind_sets[i]->set;

			vkCmdBindDescriptorSets(
				vk_command_buffer->command_buffer,
				VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
				pipeline_layout,
				0,
				vk_raytrace_pipeline->num_bind_sets,
				sets,
				0,
				nullptr
			);
		}

		vkCmdTraceRaysKHR(
			vk_command_buffer->command_buffer,
			&raygen_sbt_buffer_entry,
			&miss_sbt_buffer_entry,
			&hitgroup_sbt_buffer_entry,
			&callable_sbt_buffer_entry,
			width,
			height,
			depth
		);
	}
}
