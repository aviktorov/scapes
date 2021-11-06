#include "render/vulkan/Device.h"
#include "render/vulkan/Context.h"
#include "render/vulkan/Platform.h"
#include "render/vulkan/DescriptorSetLayoutCache.h"
#include "render/vulkan/ImageViewCache.h"
#include "render/vulkan/PipelineLayoutCache.h"
#include "render/vulkan/PipelineCache.h"
#include "render/vulkan/RenderPassBuilder.h"
#include "render/vulkan/Utils.h"

#include <scapes/foundation/Profiler.h>

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
					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
				)
				.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
				.addColorAttachmentReference(0, 0)
				.build(context->getDevice());

			// Create frame objects (semaphores, image views, framebuffers)
			for (size_t i = 0; i < swap_chain->num_images; i++)
			{
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

	render::VertexBuffer *Device::createVertexBuffer(
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
		VkMemoryPropertyFlags memory_flags = 0;

		if (type == BufferType::STATIC)
		{
			usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}
		else if (type == BufferType::DYNAMIC)
		{
			memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}

		// create vertex buffer
		Utils::createBuffer(context, buffer_size, usage_flags, memory_flags, result->buffer, result->memory);

		if (data)
		{
			if (type == BufferType::STATIC)
				Utils::fillDeviceLocalBuffer(context, result->buffer, buffer_size, data);
			else if (type == BufferType::DYNAMIC)
				Utils::fillHostVisibleBuffer(context, result->memory, buffer_size, data);
		}

		return result;
	}

	render::IndexBuffer *Device::createIndexBuffer(
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
		VkMemoryPropertyFlags memory_flags = 0;

		if (type == BufferType::STATIC)
		{
			usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}
		else if (type == BufferType::DYNAMIC)
		{
			memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}

		// create index buffer
		Utils::createBuffer(context, buffer_size, usage_flags, memory_flags, result->buffer, result->memory);

		if (data)
		{
			if (type == BufferType::STATIC)
				Utils::fillDeviceLocalBuffer(context, result->buffer, buffer_size, data);
			else if (type == BufferType::DYNAMIC)
				Utils::fillHostVisibleBuffer(context, result->memory, buffer_size, data);
		}

		return result;
	}

	render::Texture *Device::createTexture2D(
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

		return result;
	}

	render::Texture *Device::createTexture2DMultisample(
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

		return result;
	}

	render::Texture *Device::createTexture2DArray(
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

		return result;
	}

	render::Texture *Device::createTexture3D(
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

		return result;
	}

	render::Texture *Device::createTextureCube(
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

		return result;
	}

	render::FrameBuffer *Device::createFrameBuffer(
		uint32_t num_attachments,
		const FrameBufferAttachment *attachments
	)
	{
		assert(num_attachments <= FrameBuffer::MAX_ATTACHMENTS);
		assert(num_attachments == 0 || (num_attachments && attachments));

		FrameBuffer *result = new FrameBuffer();

		uint32_t width = 0;
		uint32_t height = 0;

		// add color attachments
		result->num_attachments = num_attachments;
		for (uint32_t i = 0; i < num_attachments; ++i)
		{
			const FrameBufferAttachment &input_attachment = attachments[i];
			const Texture *texture = static_cast<const Texture *>(input_attachment.texture);

			result->attachment_views[i] = texture->image_view_cache->fetch(texture, input_attachment.base_mip, 1, input_attachment.base_layer, input_attachment.num_layers);
			result->attachment_formats[i] = texture->format;
			result->attachment_samples[i] = texture->samples;

			width = std::max<int>(1, texture->width / (1 << input_attachment.base_mip));
			height = std::max<int>(1, texture->height / (1 << input_attachment.base_mip));
		}

		result->sizes.width = width;
		result->sizes.height = height;

		return result;
	}

	render::RenderPass *Device::createRenderPass(
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

		return result;
	}

	render::RenderPass *Device::createRenderPass(
		const render::SwapChain *swap_chain,
		RenderPassLoadOp load_op,
		RenderPassStoreOp store_op,
		const RenderPassClearColor *clear_color
	)
	{
		assert(swap_chain);

		const SwapChain *vk_swap_chain = static_cast<const SwapChain *>(swap_chain);

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

		if (clear_color)
		{
			result->attachment_clear_values[0].color.float32[0] = clear_color->as_f32[0];
			result->attachment_clear_values[0].color.float32[1] = clear_color->as_f32[1];
			result->attachment_clear_values[0].color.float32[2] = clear_color->as_f32[2];
			result->attachment_clear_values[0].color.float32[3] = clear_color->as_f32[3];
		}

		result->num_color_attachments = 1;
		result->max_samples = VK_SAMPLE_COUNT_1_BIT;

		RenderPassBuilder builder;
		result->render_pass = builder
			.addAttachment(vk_format, vk_samples, vk_load_op, vk_store_op, vk_load_op, vk_store_op, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
			.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
			.addColorAttachmentReference(0, 0)
			.build(context->getDevice());

		return result;
	}

	render::CommandBuffer *Device::createCommandBuffer(
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
			destroyCommandBuffer(result);
			return nullptr;
		}

		// Create synchronization primitives
		VkSemaphoreCreateInfo semaphore_info = {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(context->getDevice(), &semaphore_info, nullptr, &result->rendering_finished_gpu) != VK_SUCCESS)
		{
			std::cerr << "Device::createCommandBuffer(): can't create 'rendering finished' semaphore" << std::endl;
			destroyCommandBuffer(result);
			return nullptr;
		}

		// fences
		VkFenceCreateInfo fence_info = {};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateFence(context->getDevice(), &fence_info, nullptr, &result->rendering_finished_cpu) != VK_SUCCESS)
		{
			std::cerr << "Device::createCommandBuffer(): can't create 'rendering finished' fence" << std::endl;
			destroyCommandBuffer(result);
			return nullptr;
		}

		return result;
	}

	render::UniformBuffer *Device::createUniformBuffer(
		BufferType type,
		uint32_t size,
		const void *data
	)
	{
		assert(type == BufferType::DYNAMIC && "Only dynamic buffers are implemented at the moment");
		assert(size != 0 && "Invalid size");

		UniformBuffer *result = new UniformBuffer();
		result->type = type;
		result->size = size;

		Utils::createBuffer(
			context,
			size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			result->buffer,
			result->memory
		);

		if (data != nullptr)
			Utils::fillHostVisibleBuffer(context, result->memory, size, data);

		return result;
	}

	render::Shader *Device::createShaderFromSource(
		ShaderType type,
		uint32_t size,
		const char *data,
		const char *path
	)
	{
		assert(false && "Not implemented");
		return nullptr;
	}

	render::Shader *Device::createShaderFromIL(
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

		return result;
	}

	render::BindSet *Device::createBindSet()
	{
		BindSet *result = new BindSet();
		memset(result, 0, sizeof(BindSet));

		return result;
	}

	render::PipelineState *Device::createPipelineState()
	{
		PipelineState *result = new PipelineState();
		memset(result, 0, sizeof(PipelineState));

		result->cull_mode = VK_CULL_MODE_BACK_BIT;
		result->primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		result->depth_compare_func = VK_COMPARE_OP_LESS_OR_EQUAL;

		return result;
	}

	render::SwapChain *Device::createSwapChain(void *native_window)
	{
		assert(native_window != nullptr && "Invalid window");

		SwapChain *result = new SwapChain();

		// Create platform surface
		result->surface = Platform::createSurface(context, native_window);
		if (result->surface == VK_NULL_HANDLE)
		{
			std::cerr << "Device::createSwapChain(): can't create platform surface" << std::endl;
			destroySwapChain(result);
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
			destroySwapChain(result);
			return false;
		}

		helpers::selectOptimalSwapChainSettings(context, result);
		helpers::createSwapChainObjects(context, result);

		return result;
	}

	void Device::destroyVertexBuffer(render::VertexBuffer *vertex_buffer)
	{
		if (vertex_buffer == nullptr)
			return;

		VertexBuffer *vk_vertex_buffer = static_cast<VertexBuffer *>(vertex_buffer);

		vmaDestroyBuffer(context->getVRAMAllocator(), vk_vertex_buffer->buffer, vk_vertex_buffer->memory);

		vk_vertex_buffer->buffer = VK_NULL_HANDLE;
		vk_vertex_buffer->memory = VK_NULL_HANDLE;

		delete vk_vertex_buffer;
		vk_vertex_buffer = nullptr;
	}

	void Device::destroyIndexBuffer(render::IndexBuffer *index_buffer)
	{
		if (index_buffer == nullptr)
			return;

		IndexBuffer *vk_index_buffer = static_cast<IndexBuffer *>(index_buffer);

		vmaDestroyBuffer(context->getVRAMAllocator(), vk_index_buffer->buffer, vk_index_buffer->memory);

		vk_index_buffer->buffer = VK_NULL_HANDLE;
		vk_index_buffer->memory = VK_NULL_HANDLE;

		delete vk_index_buffer;
		vk_index_buffer = nullptr;
	}

	void Device::destroyTexture(render::Texture *texture)
	{
		if (texture == nullptr)
			return;

		Texture *vk_texture = static_cast<Texture *>(texture);

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

	void Device::destroyFrameBuffer(render::FrameBuffer *frame_buffer)
	{
		if (frame_buffer == nullptr)
			return;

		FrameBuffer *vk_frame_buffer = static_cast<FrameBuffer *>(frame_buffer);

		for (uint8_t i = 0; i < vk_frame_buffer->num_attachments; ++i)
			vk_frame_buffer->attachment_views[i] = VK_NULL_HANDLE;

		vkDestroyFramebuffer(context->getDevice(), vk_frame_buffer->frame_buffer, nullptr);
		vk_frame_buffer->frame_buffer = VK_NULL_HANDLE;

		delete vk_frame_buffer;
		vk_frame_buffer = nullptr;
	}

	void Device::destroyRenderPass(render::RenderPass *render_pass)
	{
		if (render_pass == nullptr)
			return;

		RenderPass *vk_render_pass = static_cast<RenderPass *>(render_pass);

		vkDestroyRenderPass(context->getDevice(), vk_render_pass->render_pass, nullptr);
		vk_render_pass->render_pass = VK_NULL_HANDLE;

		delete vk_render_pass;
		vk_render_pass = nullptr;
	}

	void Device::destroyCommandBuffer(render::CommandBuffer *command_buffer)
	{
		if (command_buffer == nullptr)
			return;

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);

		vkFreeCommandBuffers(context->getDevice(), context->getCommandPool(), 1, &vk_command_buffer->command_buffer);
		vk_command_buffer->command_buffer = VK_NULL_HANDLE;

		vkDestroySemaphore(context->getDevice(), vk_command_buffer->rendering_finished_gpu, nullptr);
		vk_command_buffer->rendering_finished_gpu = VK_NULL_HANDLE;

		vkDestroyFence(context->getDevice(), vk_command_buffer->rendering_finished_cpu, nullptr);
		vk_command_buffer->rendering_finished_cpu = VK_NULL_HANDLE;

		delete vk_command_buffer;
		vk_command_buffer = nullptr;
	}

	void Device::destroyUniformBuffer(render::UniformBuffer *uniform_buffer)
	{
		if (uniform_buffer == nullptr)
			return;

		UniformBuffer *vk_uniform_buffer = static_cast<UniformBuffer *>(uniform_buffer);

		vmaDestroyBuffer(context->getVRAMAllocator(), vk_uniform_buffer->buffer, vk_uniform_buffer->memory);

		vk_uniform_buffer->buffer = VK_NULL_HANDLE;
		vk_uniform_buffer->memory = VK_NULL_HANDLE;

		delete vk_uniform_buffer;
		vk_uniform_buffer = nullptr;
	}

	void Device::destroyShader(render::Shader *shader)
	{
		if (shader == nullptr)
			return;

		Shader *vk_shader = static_cast<Shader *>(shader);

		vkDestroyShaderModule(context->getDevice(), vk_shader->module, nullptr);
		vk_shader->module = VK_NULL_HANDLE;

		delete vk_shader;
		vk_shader = nullptr;
	}

	void Device::destroyBindSet(render::BindSet *bind_set)
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
		}

		if (vk_bind_set->set != VK_NULL_HANDLE)
			vkFreeDescriptorSets(context->getDevice(), context->getDescriptorPool(), 1, &vk_bind_set->set);

		delete vk_bind_set;
		vk_bind_set = nullptr;
	}

	void Device::destroyPipelineState(render::PipelineState *pipeline_state)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		delete vk_pipeline_state;
		vk_pipeline_state = nullptr;
	}

	void Device::destroySwapChain(render::SwapChain *swap_chain)
	{
		if (swap_chain == nullptr)
			return;

		SwapChain *vk_swap_chain = static_cast<SwapChain *>(swap_chain);

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

	uint32_t Device::getNumSwapChainImages(const render::SwapChain *swap_chain)
	{
		assert(swap_chain != nullptr && "Invalid swap chain");
		const SwapChain *vk_swap_chain = static_cast<const SwapChain *>(swap_chain);

		return vk_swap_chain->num_images;
	}

	void Device::setTextureSamplerWrapMode(render::Texture *texture, SamplerWrapMode mode)
	{
		assert(texture != nullptr && "Invalid texture");
		Texture *vk_texture = static_cast<Texture *>(texture);

		vkDestroySampler(context->getDevice(), vk_texture->sampler, nullptr);
		vk_texture->sampler = VK_NULL_HANDLE;

		VkSamplerAddressMode sampler_mode = Utils::getSamplerAddressMode(mode);
		vk_texture->sampler = Utils::createSampler(context, 0, vk_texture->num_mipmaps, sampler_mode, sampler_mode, sampler_mode);
	}

	void Device::setTextureSamplerDepthCompare(render::Texture *texture, bool enabled, DepthCompareFunc func)
	{
		assert(texture != nullptr && "Invalid texture");
		Texture *vk_texture = static_cast<Texture *>(texture);

		vkDestroySampler(context->getDevice(), vk_texture->sampler, nullptr);
		vk_texture->sampler = VK_NULL_HANDLE;

		VkSamplerAddressMode sampler_mode = Utils::getSamplerAddressMode(SamplerWrapMode::REPEAT);
		VkCompareOp compare_func = Utils::getDepthCompareFunc(func);
		vk_texture->sampler = Utils::createSampler(context, 0, vk_texture->num_mipmaps, sampler_mode, sampler_mode, sampler_mode, enabled, compare_func);
	}

	void Device::generateTexture2DMipmaps(render::Texture *texture)
	{
		assert(texture != nullptr && "Invalid texture");

		Texture *vk_texture = static_cast<Texture *>(texture);

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
	void *Device::map(render::VertexBuffer *vertex_buffer)
	{
		assert(vertex_buffer != nullptr && "Invalid buffer");

		VertexBuffer *vk_vertex_buffer = static_cast<VertexBuffer *>(vertex_buffer);
		assert(vk_vertex_buffer->type == BufferType::DYNAMIC && "Mapped buffer must have BufferType::DYNAMIC type");

		void *result = nullptr;
		if (vmaMapMemory(context->getVRAMAllocator(), vk_vertex_buffer->memory, &result) != VK_SUCCESS)
		{
			// TODO: log error
		}

		return result;
	}

	void Device::unmap(render::VertexBuffer *vertex_buffer)
	{
		assert(vertex_buffer != nullptr && "Invalid buffer");

		VertexBuffer *vk_vertex_buffer = static_cast<VertexBuffer *>(vertex_buffer);
		assert(vk_vertex_buffer->type == BufferType::DYNAMIC && "Mapped buffer must have BufferType::DYNAMIC type");

		vmaUnmapMemory(context->getVRAMAllocator(), vk_vertex_buffer->memory);
	}

	void *Device::map(render::IndexBuffer *index_buffer)
	{
		assert(index_buffer != nullptr && "Invalid uniform buffer");

		IndexBuffer *vk_index_buffer = static_cast<IndexBuffer *>(index_buffer);
		assert(vk_index_buffer->type == BufferType::DYNAMIC && "Mapped buffer must have BufferType::DYNAMIC type");

		void *result = nullptr;
		if (vmaMapMemory(context->getVRAMAllocator(), vk_index_buffer->memory, &result) != VK_SUCCESS)
		{
			// TODO: log error
		}

		return result;
	}

	void Device::unmap(render::IndexBuffer *index_buffer)
	{
		assert(index_buffer != nullptr && "Invalid buffer");

		IndexBuffer *vk_index_buffer = static_cast<IndexBuffer *>(index_buffer);
		assert(vk_index_buffer->type == BufferType::DYNAMIC && "Mapped buffer must have BufferType::DYNAMIC type");

		vmaUnmapMemory(context->getVRAMAllocator(), vk_index_buffer->memory);
	}

	void *Device::map(render::UniformBuffer *uniform_buffer)
	{
		assert(uniform_buffer != nullptr && "Invalid uniform buffer");

		UniformBuffer *vk_uniform_buffer = static_cast<UniformBuffer *>(uniform_buffer);
		assert(vk_uniform_buffer->type == BufferType::DYNAMIC && "Mapped buffer must have BufferType::DYNAMIC type");

		void *result = nullptr;
		if (vmaMapMemory(context->getVRAMAllocator(), vk_uniform_buffer->memory, &result) != VK_SUCCESS)
		{
			// TODO: log error
		}

		return result;
	}

	void Device::unmap(render::UniformBuffer *uniform_buffer)
	{
		assert(uniform_buffer != nullptr && "Invalid buffer");

		UniformBuffer *vk_uniform_buffer = static_cast<UniformBuffer *>(uniform_buffer);
		assert(vk_uniform_buffer->type == BufferType::DYNAMIC && "Mapped buffer must have BufferType::DYNAMIC type");

		vmaUnmapMemory(context->getVRAMAllocator(), vk_uniform_buffer->memory);
	}

	/*
	 */
	void Device::flush(render::BindSet *bind_set)
	{
		if (bind_set == nullptr)
			return;

		BindSet *vk_bind_set = static_cast<BindSet *>(bind_set);

		VkWriteDescriptorSet writes[BindSet::MAX_BINDINGS];
		VkDescriptorImageInfo image_infos[BindSet::MAX_BINDINGS];
		VkDescriptorBufferInfo buffer_infos[BindSet::MAX_BINDINGS];

		uint32_t write_size = 0;
		uint32_t image_size = 0;
		uint32_t buffer_size = 0;

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
				{
					VkDescriptorImageInfo info = {};
					info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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

	void Device::flush(render::PipelineState *pipeline_state)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		for (uint32_t i = 0; i < vk_pipeline_state->num_bind_sets; ++i)
			flush(vk_pipeline_state->bind_sets[i]);

		if (vk_pipeline_state->pipeline_layout == VK_NULL_HANDLE)
		{
			vk_pipeline_state->pipeline_layout = pipeline_layout_cache->fetch(vk_pipeline_state);
			vk_pipeline_state->pipeline = VK_NULL_HANDLE;
		}

		if (vk_pipeline_state->pipeline == VK_NULL_HANDLE)
			vk_pipeline_state->pipeline = pipeline_cache->fetch(vk_pipeline_state->pipeline_layout, vk_pipeline_state);
	}

	/*
	 */
	bool Device::acquire(render::SwapChain *swap_chain, uint32_t *new_image)
	{
		SwapChain *vk_swap_chain = static_cast<SwapChain *>(swap_chain);

		vk_swap_chain->current_frame++;
		vk_swap_chain->current_frame %= vk_swap_chain->num_images;

		VkSemaphore sync = vk_swap_chain->image_available_gpu[vk_swap_chain->current_frame];

		VkResult result = vkAcquireNextImageKHR(
			context->getDevice(),
			vk_swap_chain->swap_chain,
			std::numeric_limits<uint64_t>::max(),
			sync,
			VK_NULL_HANDLE,
			&vk_swap_chain->current_image
		);

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

	bool Device::present(render::SwapChain *swap_chain, uint32_t num_wait_command_buffers, render::CommandBuffer * const *wait_command_buffers)
	{
		SwapChain *vk_swap_chain = static_cast<SwapChain *>(swap_chain);

		std::vector<VkSemaphore> wait_semaphores(num_wait_command_buffers);

		if (num_wait_command_buffers != 0 && wait_command_buffers != nullptr)
		{
			for (uint32_t i = 0; i < num_wait_command_buffers; ++i)
			{
				const CommandBuffer *vk_wait_command_buffer = static_cast<const CommandBuffer *>(wait_command_buffers[i]);
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
		render::CommandBuffer * const *wait_command_buffers
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

		VkResult result = vkWaitForFences(context->getDevice(), num_wait_command_buffers, wait_fences.data(), VK_TRUE, UINT64_MAX);

		return result == VK_SUCCESS;
	}

	/*
	 */
	void Device::bindUniformBuffer(
		render::BindSet *bind_set,
		uint32_t binding,
		const render::UniformBuffer *uniform_buffer
	)
	{
		assert(binding < BindSet::MAX_BINDINGS);

		if (bind_set == nullptr)
			return;

		BindSet *vk_bind_set = static_cast<BindSet *>(bind_set);
		const UniformBuffer *vk_uniform_buffer = static_cast<const UniformBuffer *>(uniform_buffer);
		
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
		render::BindSet *bind_set,
		uint32_t binding,
		const render::Texture *texture
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

	void Device::bindTexture(
		render::BindSet *bind_set,
		uint32_t binding,
		const render::Texture *texture,
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

		info.binding = binding;
		info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		info.descriptorCount = 1;
		info.stageFlags = VK_SHADER_STAGE_ALL; // TODO: allow for different shader stages
		info.pImmutableSamplers = nullptr;
	}

	/*
	 */
	void Device::clearPushConstants(render::PipelineState *pipeline_state)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		vk_pipeline_state->push_constants_size = 0;
		memset(vk_pipeline_state->push_constants, 0, PipelineState::MAX_PUSH_CONSTANT_SIZE);

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_pipeline_state->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::setPushConstants(render::PipelineState *pipeline_state, uint8_t size, const void *data)
	{
		assert(size <= PipelineState::MAX_PUSH_CONSTANT_SIZE);

		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		vk_pipeline_state->push_constants_size = size;
		memcpy(vk_pipeline_state->push_constants, data, size);

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_pipeline_state->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::clearBindSets(render::PipelineState *pipeline_state)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		for (uint32_t i = 0; i < PipelineState::MAX_BIND_SETS; ++i)
			vk_pipeline_state->bind_sets[i] = nullptr;

		vk_pipeline_state->num_bind_sets = 0;

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_pipeline_state->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::setBindSet(render::PipelineState *pipeline_state, uint8_t binding, render::BindSet *bind_set)
	{
		assert(binding < PipelineState::MAX_BIND_SETS);

		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);
		BindSet *vk_bind_set = static_cast<BindSet *>(bind_set);

		vk_pipeline_state->bind_sets[binding] = vk_bind_set;
		vk_pipeline_state->num_bind_sets = std::max<uint32_t>(vk_pipeline_state->num_bind_sets, binding + 1);

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_pipeline_state->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::clearShaders(render::PipelineState *pipeline_state)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		for (uint32_t i = 0; i < PipelineState::MAX_SHADERS; ++i)
			vk_pipeline_state->shaders[i] = VK_NULL_HANDLE;

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_pipeline_state->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::setShader(render::PipelineState *pipeline_state, ShaderType type, const render::Shader *shader)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);
		const Shader *vk_shader = static_cast<const Shader *>(shader);

		vk_pipeline_state->shaders[static_cast<int>(type)] = (vk_shader) ? vk_shader->module : VK_NULL_HANDLE;

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_pipeline_state->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::clearVertexStreams(render::PipelineState *pipeline_state)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		for (uint32_t i = 0; i < PipelineState::MAX_VERTEX_STREAMS; ++i)
			vk_pipeline_state->vertex_streams[i] = nullptr;

		vk_pipeline_state->num_vertex_streams = 0;

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_pipeline_state->pipeline_layout = VK_NULL_HANDLE;
	}

	void Device::setVertexStream(render::PipelineState *pipeline_state, uint8_t binding, render::VertexBuffer *vertex_buffer)
	{
		assert(binding < PipelineState::MAX_VERTEX_STREAMS);

		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);
		VertexBuffer *vk_vertex_buffer = static_cast<VertexBuffer *>(vertex_buffer);

		vk_pipeline_state->vertex_streams[binding] = vk_vertex_buffer;
		vk_pipeline_state->num_vertex_streams = std::max<uint32_t>(vk_pipeline_state->num_vertex_streams, binding + 1);

		// TODO: better invalidation (there might be case where we only need to invalidate pipeline but keep pipeline layout)
		vk_pipeline_state->pipeline_layout = VK_NULL_HANDLE;
	}

	/*
	 */
	void Device::setViewport(render::PipelineState *pipeline_state, int32_t x, int32_t y, uint32_t width, uint32_t height)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		vk_pipeline_state->viewport.x = static_cast<float>(x);
		vk_pipeline_state->viewport.y = static_cast<float>(y);
		vk_pipeline_state->viewport.width = static_cast<float>(width);
		vk_pipeline_state->viewport.height = static_cast<float>(height);
		vk_pipeline_state->viewport.minDepth = 0.0f;
		vk_pipeline_state->viewport.maxDepth = 1.0f;

		vk_pipeline_state->pipeline = VK_NULL_HANDLE;
	}

	void Device::setScissor(render::PipelineState *pipeline_state, int32_t x, int32_t y, uint32_t width, uint32_t height)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		vk_pipeline_state->scissor.offset = { x, y };
		vk_pipeline_state->scissor.extent = { width, height };

		vk_pipeline_state->pipeline = VK_NULL_HANDLE;
	}

	void Device::setPrimitiveType(render::PipelineState *pipeline_state, RenderPrimitiveType type)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		vk_pipeline_state->primitive_topology = Utils::getPrimitiveTopology(type);

		vk_pipeline_state->pipeline = VK_NULL_HANDLE;
	}

	void Device::setCullMode(render::PipelineState *pipeline_state, CullMode mode)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		vk_pipeline_state->cull_mode = Utils::getCullMode(mode);

		vk_pipeline_state->pipeline = VK_NULL_HANDLE;
	}

	void Device::setDepthTest(render::PipelineState *pipeline_state, bool enabled)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		vk_pipeline_state->depth_test = enabled;

		vk_pipeline_state->pipeline = VK_NULL_HANDLE;
	}

	void Device::setDepthWrite(render::PipelineState *pipeline_state, bool enabled)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		vk_pipeline_state->depth_write = enabled;

		vk_pipeline_state->pipeline = VK_NULL_HANDLE;
	}

	void Device::setDepthCompareFunc(render::PipelineState *pipeline_state, DepthCompareFunc func)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		vk_pipeline_state->depth_compare_func = Utils::getDepthCompareFunc(func);

		vk_pipeline_state->pipeline = VK_NULL_HANDLE;
	}

	void Device::setBlending(render::PipelineState *pipeline_state, bool enabled)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		vk_pipeline_state->blending = enabled;

		vk_pipeline_state->pipeline = VK_NULL_HANDLE;
	}

	void Device::setBlendFactors(render::PipelineState *pipeline_state, BlendFactor src_factor, BlendFactor dst_factor)
	{
		if (pipeline_state == nullptr)
			return;

		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);

		vk_pipeline_state->blend_src_factor = Utils::getBlendFactor(src_factor);
		vk_pipeline_state->blend_dst_factor = Utils::getBlendFactor(dst_factor);

		vk_pipeline_state->pipeline = VK_NULL_HANDLE;
	}

	/*
	 */
	bool Device::resetCommandBuffer(render::CommandBuffer *command_buffer)
	{
		if (command_buffer == nullptr)
			return false;

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		if (vkResetCommandBuffer(vk_command_buffer->command_buffer, 0) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Device::beginCommandBuffer(render::CommandBuffer *command_buffer)
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

	bool Device::endCommandBuffer(render::CommandBuffer *command_buffer)
	{
		if (command_buffer == nullptr)
			return false;

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		if (vkEndCommandBuffer(vk_command_buffer->command_buffer) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Device::submit(render::CommandBuffer *command_buffer)
	{
		if (command_buffer == nullptr)
			return false;
		
		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);

		VkSubmitInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &vk_command_buffer->command_buffer;

		vkResetFences(context->getDevice(), 1, &vk_command_buffer->rendering_finished_cpu);
		if (vkQueueSubmit(context->getGraphicsQueue(), 1, &info, vk_command_buffer->rendering_finished_cpu) != VK_SUCCESS)
			return false;

		return true;
	}

	bool Device::submitSyncked(render::CommandBuffer *command_buffer, const render::SwapChain *wait_swap_chain)
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

	bool Device::submitSyncked(render::CommandBuffer *command_buffer, uint32_t num_wait_command_buffers, render::CommandBuffer * const *wait_command_buffers)
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

	void Device::beginRenderPass(render::CommandBuffer *command_buffer, const render::RenderPass *render_pass, const render::FrameBuffer *frame_buffer)
	{
		assert(command_buffer);
		assert(render_pass);
		assert(frame_buffer);

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		const RenderPass *vk_render_pass = static_cast<const RenderPass *>(render_pass);
		const FrameBuffer *vk_frame_buffer = static_cast<const FrameBuffer *>(frame_buffer);

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
			framebuffer_info.layers = 1;

			if (vkCreateFramebuffer(context->getDevice(), &framebuffer_info, nullptr, &vk_frame_buffer->frame_buffer) != VK_SUCCESS)
			{
				vk_frame_buffer->frame_buffer = VK_NULL_HANDLE;
				// TODO: log error
				return;
			}
		}

		VkViewport viewport = {};
		viewport.width = static_cast<float>(vk_frame_buffer->sizes.width);
		viewport.height = static_cast<float>(vk_frame_buffer->sizes.height);
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

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

	void Device::beginRenderPass(render::CommandBuffer *command_buffer, const render::RenderPass *render_pass, const render::SwapChain *swap_chain)
	{
		assert(command_buffer);
		assert(render_pass);
		assert(swap_chain);

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		const RenderPass *vk_render_pass = static_cast<const RenderPass *>(render_pass);
		const SwapChain *vk_swap_chain = static_cast<const SwapChain *>(swap_chain);
		VkFramebuffer frame_buffer = vk_swap_chain->frame_buffers[vk_swap_chain->current_image];

		VkViewport viewport = {};
		viewport.width = static_cast<float>(vk_swap_chain->sizes.width);
		viewport.height = static_cast<float>(vk_swap_chain->sizes.height);
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

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

	void Device::endRenderPass(render::CommandBuffer *command_buffer)
	{
		if (command_buffer == nullptr)
			return;

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		vkCmdEndRenderPass(vk_command_buffer->command_buffer);

		vk_command_buffer->render_pass = VK_NULL_HANDLE;
	}

	void Device::drawIndexedPrimitiveInstanced(
		render::CommandBuffer *command_buffer,
		render::PipelineState *pipeline_state,
		const render::IndexBuffer *index_buffer,
		uint32_t num_indices,
		uint32_t base_index,
		int32_t base_vertex,
		uint32_t num_instances,
		uint32_t base_instance
	)
	{
		SCAPES_PROFILER_SCOPED;

		if (command_buffer == nullptr)
			return;

		CommandBuffer *vk_command_buffer = static_cast<CommandBuffer *>(command_buffer);
		PipelineState *vk_pipeline_state = static_cast<PipelineState *>(pipeline_state);
		const IndexBuffer *vk_index_buffer = static_cast<const IndexBuffer *>(index_buffer);

		assert(vk_command_buffer->render_pass != VK_NULL_HANDLE);

		vk_pipeline_state->render_pass = vk_command_buffer->render_pass;
		vk_pipeline_state->num_color_attachments = vk_command_buffer->num_color_attachments;
		vk_pipeline_state->max_samples = vk_command_buffer->max_samples;

		flush(pipeline_state);

		SCAPES_PROFILER_NAMED_N(actual_draw_call, "Actual draw call");

		VkPipeline pipeline = vk_pipeline_state->pipeline;
		VkPipelineLayout pipeline_layout = vk_pipeline_state->pipeline_layout;
		VkViewport viewport = vk_pipeline_state->viewport;
		VkRect2D scissor = vk_pipeline_state->scissor;

		vkCmdSetViewport(vk_command_buffer->command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(vk_command_buffer->command_buffer, 0, 1, &scissor);

		vkCmdBindPipeline(vk_command_buffer->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		if (vk_pipeline_state->push_constants_size > 0)
			vkCmdPushConstants(vk_command_buffer->command_buffer, pipeline_layout, VK_SHADER_STAGE_ALL, 0, vk_pipeline_state->push_constants_size, vk_pipeline_state->push_constants);

		if (vk_pipeline_state->num_bind_sets > 0)
		{
			VkDescriptorSet sets[PipelineState::MAX_BIND_SETS];
			for (uint32_t i = 0; i < vk_pipeline_state->num_bind_sets; ++i)
				sets[i] = vk_pipeline_state->bind_sets[i]->set;

			vkCmdBindDescriptorSets(vk_command_buffer->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, vk_pipeline_state->num_bind_sets, sets, 0, nullptr);
		}

		if (vk_pipeline_state->num_vertex_streams > 0)
		{
			VkBuffer vertex_buffers[PipelineState::MAX_VERTEX_STREAMS];
			VkDeviceSize offsets[PipelineState::MAX_VERTEX_STREAMS];

			for (uint32_t i = 0; i < vk_pipeline_state->num_vertex_streams; ++i)
			{
				vertex_buffers[i] = vk_pipeline_state->vertex_streams[i]->buffer;
				offsets[i] = 0;
			}

			vkCmdBindVertexBuffers(vk_command_buffer->command_buffer, 0, vk_pipeline_state->num_vertex_streams, vertex_buffers, offsets);
		}

		if (vk_index_buffer)
			vkCmdBindIndexBuffer(vk_command_buffer->command_buffer, vk_index_buffer->buffer, 0, vk_index_buffer->index_type);

		vkCmdDrawIndexed(vk_command_buffer->command_buffer, num_indices, num_instances, base_index, base_vertex, base_instance);
	}
}
