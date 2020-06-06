#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

#include "render/backend/vulkan/driver.h"
#include "render/backend/vulkan/platform.h"
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
				VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_B8G8R8_SNORM, VK_FORMAT_B8G8R8_UINT, VK_FORMAT_B8G8R8_SINT,
				VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_SINT,
				VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM, VK_FORMAT_B8G8R8A8_UINT, VK_FORMAT_B8G8R8A8_SINT,

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

		static Format fromFormat(VkFormat format)
		{
			switch (format)
			{
				case VK_FORMAT_UNDEFINED: return Format::UNDEFINED;

				case VK_FORMAT_R8_UNORM: return Format::R8_UNORM;
				case VK_FORMAT_R8_SNORM: return Format::R8_SNORM;
				case VK_FORMAT_R8_UINT: return Format::R8_UINT;
				case VK_FORMAT_R8_SINT: return Format::R8_SINT;
				case VK_FORMAT_R8G8_UNORM: return Format::R8G8_UNORM;
				case VK_FORMAT_R8G8_SNORM: return Format::R8G8_SNORM;
				case VK_FORMAT_R8G8_UINT: return Format::R8G8_UINT;
				case VK_FORMAT_R8G8_SINT: return Format::R8G8_SINT;
				case VK_FORMAT_R8G8B8_UNORM: return Format::R8G8B8_UNORM;
				case VK_FORMAT_R8G8B8_SNORM: return Format::R8G8B8_SNORM;
				case VK_FORMAT_R8G8B8_UINT: return Format::R8G8B8_UINT;
				case VK_FORMAT_R8G8B8_SINT: return Format::R8G8B8_SINT;
				case VK_FORMAT_B8G8R8_UNORM: return Format::B8G8R8_UNORM;
				case VK_FORMAT_B8G8R8_SNORM: return Format::B8G8R8_SNORM;
				case VK_FORMAT_B8G8R8_UINT: return Format::B8G8R8_UINT;
				case VK_FORMAT_B8G8R8_SINT: return Format::B8G8R8_SINT;
				case VK_FORMAT_R8G8B8A8_UNORM: return Format::R8G8B8A8_UNORM;
				case VK_FORMAT_R8G8B8A8_SNORM: return Format::R8G8B8A8_SNORM;
				case VK_FORMAT_R8G8B8A8_UINT: return Format::R8G8B8A8_UINT;
				case VK_FORMAT_R8G8B8A8_SINT: return Format::R8G8B8A8_SINT;
				case VK_FORMAT_B8G8R8A8_UNORM: return Format::B8G8R8A8_UNORM;
				case VK_FORMAT_B8G8R8A8_SNORM: return Format::B8G8R8A8_SNORM;
				case VK_FORMAT_B8G8R8A8_UINT: return Format::B8G8R8A8_UINT;
				case VK_FORMAT_B8G8R8A8_SINT: return Format::B8G8R8A8_SINT;

				case VK_FORMAT_R16_UNORM: return Format::R16_UNORM;
				case VK_FORMAT_R16_SNORM: return Format::R16_SNORM;
				case VK_FORMAT_R16_UINT: return Format::R16_UINT;
				case VK_FORMAT_R16_SINT: return Format::R16_SINT;
				case VK_FORMAT_R16_SFLOAT: return Format::R16_SFLOAT;
				case VK_FORMAT_R16G16_UNORM: return Format::R16G16_UNORM;
				case VK_FORMAT_R16G16_SNORM: return Format::R16G16_SNORM;
				case VK_FORMAT_R16G16_UINT: return Format::R16G16_UINT;
				case VK_FORMAT_R16G16_SINT: return Format::R16G16_SINT;
				case VK_FORMAT_R16G16_SFLOAT: return Format::R16G16_SFLOAT;
				case VK_FORMAT_R16G16B16_UNORM: return Format::R16G16B16_UNORM;
				case VK_FORMAT_R16G16B16_SNORM: return Format::R16G16B16_SNORM;
				case VK_FORMAT_R16G16B16_UINT: return Format::R16G16B16_UINT;
				case VK_FORMAT_R16G16B16_SINT: return Format::R16G16B16_SINT;
				case VK_FORMAT_R16G16B16_SFLOAT: return Format::R16G16B16_SFLOAT;
				case VK_FORMAT_R16G16B16A16_UNORM: return Format::R16G16B16A16_UNORM;
				case VK_FORMAT_R16G16B16A16_SNORM: return Format::R16G16B16A16_SNORM;
				case VK_FORMAT_R16G16B16A16_UINT: return Format::R16G16B16A16_UINT;
				case VK_FORMAT_R16G16B16A16_SINT: return Format::R16G16B16A16_SINT;
				case VK_FORMAT_R16G16B16A16_SFLOAT: return Format::R16G16B16A16_SFLOAT;

				case VK_FORMAT_R32_UINT: return Format::R32_UINT;
				case VK_FORMAT_R32_SINT: return Format::R32_SINT;
				case VK_FORMAT_R32_SFLOAT: return Format::R32_SFLOAT;
				case VK_FORMAT_R32G32_UINT: return Format::R32G32_UINT;
				case VK_FORMAT_R32G32_SINT: return Format::R32G32_SINT;
				case VK_FORMAT_R32G32_SFLOAT: return Format::R32G32_SFLOAT;
				case VK_FORMAT_R32G32B32_UINT: return Format::R32G32B32_UINT;
				case VK_FORMAT_R32G32B32_SINT: return Format::R32G32B32_SINT;
				case VK_FORMAT_R32G32B32_SFLOAT: return Format::R32G32B32_SFLOAT;
				case VK_FORMAT_R32G32B32A32_UINT: return Format::R32G32B32A32_UINT;
				case VK_FORMAT_R32G32B32A32_SINT: return Format::R32G32B32A32_SINT;
				case VK_FORMAT_R32G32B32A32_SFLOAT: return Format::R32G32B32A32_SFLOAT;

				case VK_FORMAT_D16_UNORM: return Format::D16_UNORM;
				case VK_FORMAT_D16_UNORM_S8_UINT: return Format::D16_UNORM_S8_UINT;
				case VK_FORMAT_D24_UNORM_S8_UINT: return Format::D24_UNORM_S8_UINT;
				case VK_FORMAT_D32_SFLOAT: return Format::D32_SFLOAT;
				case VK_FORMAT_D32_SFLOAT_S8_UINT: return Format::D32_SFLOAT_S8_UINT;

				default:
				{
					std::cerr << "vulkan::fromFormat(): unsupported format " << format << std::endl;
					return Format::UNDEFINED;
				}
			}
		}

		static VkSampleCountFlagBits toSamples(Multisample samples)
		{
			static VkSampleCountFlagBits supported_samples[static_cast<int>(Multisample::MAX)] =
			{
				VK_SAMPLE_COUNT_1_BIT, VK_SAMPLE_COUNT_2_BIT,
				VK_SAMPLE_COUNT_4_BIT, VK_SAMPLE_COUNT_8_BIT,
				VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_32_BIT,
				VK_SAMPLE_COUNT_64_BIT,
			};

			return supported_samples[static_cast<int>(samples)];
		}

		static Multisample fromSamples(VkSampleCountFlagBits samples)
		{
			if (samples & VK_SAMPLE_COUNT_64_BIT) { return Multisample::COUNT_64; }
			if (samples & VK_SAMPLE_COUNT_32_BIT) { return Multisample::COUNT_32; }
			if (samples & VK_SAMPLE_COUNT_16_BIT) { return Multisample::COUNT_16; }
			if (samples & VK_SAMPLE_COUNT_8_BIT) { return Multisample::COUNT_8; }
			if (samples & VK_SAMPLE_COUNT_4_BIT) { return Multisample::COUNT_4; }
			if (samples & VK_SAMPLE_COUNT_2_BIT) { return Multisample::COUNT_2; }

			return Multisample::COUNT_1;
		}

		static VkImageUsageFlags toImageUsageFlags(VkFormat format)
		{
			if (format == VK_FORMAT_UNDEFINED)
				return 0;

			switch (format)
			{
				case VK_FORMAT_D16_UNORM:
				case VK_FORMAT_D32_SFLOAT:
				case VK_FORMAT_D16_UNORM_S8_UINT:
				case VK_FORMAT_D24_UNORM_S8_UINT:
				case VK_FORMAT_D32_SFLOAT_S8_UINT: return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			}

			return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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
				3, 3, 3, 3,
				4, 4, 4, 4,
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
			VkImageUsageFlags usage_flags = toImageUsageFlags(texture->format);

			VulkanUtils::createImage(
				context,
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
				VulkanUtils::transitionImageLayout(
					context,
					texture->image,
					texture->format,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					0, texture->num_mipmaps,
					0, texture->num_layers
				);

				// transfer data to GPU
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

				source_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			}

			// prepare for shader access
			VulkanUtils::transitionImageLayout(
				context,
				texture->image,
				texture->format,
				source_layout,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				0, texture->num_mipmaps,
				0, texture->num_layers
			);

			// create base view & sampler
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

		static void selectOptimalSwapChainSettings(const VulkanContext *context, SwapChain *swap_chain, uint32_t width, uint32_t height)
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

		static bool createTransientSwapChainObjects(const VulkanContext *context, SwapChain *swap_chain)
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

			if (context->getGraphicsQueueFamily() != swap_chain->present_queue_family)
			{
				uint32_t families[] = { context->getGraphicsQueueFamily(), swap_chain->present_queue_family };
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

			if (vkCreateSwapchainKHR(context->getDevice(), &info, nullptr, &swap_chain->swap_chain) != VK_SUCCESS)
			{
				std::cerr << "vulkan::createTransientSwapChainObjects(): vkCreateSwapchainKHR failed" << std::endl;
				return false;
			}

			// Get surface images
			vkGetSwapchainImagesKHR(context->getDevice(), swap_chain->swap_chain, &swap_chain->num_images, nullptr);
			assert(swap_chain->num_images != 0 && swap_chain->num_images < SwapChain::MAX_IMAGES);
			vkGetSwapchainImagesKHR(context->getDevice(), swap_chain->swap_chain, &swap_chain->num_images, swap_chain->images);

			// Create image views
			for (size_t i = 0; i < swap_chain->num_images; i++)
				swap_chain->views[i] = VulkanUtils::createImageView(
					context,
					swap_chain->images[i],
					swap_chain->surface_format.format,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_VIEW_TYPE_2D
				);

			return true;
		}

		static void destroyTransientSwapChainObjects(const VulkanContext *context, SwapChain *swap_chain)
		{
			for (size_t i = 0; i < swap_chain->num_images; ++i)
			{
				vkDestroyImageView(context->getDevice(), swap_chain->views[i], nullptr);
				swap_chain->images[i] = VK_NULL_HANDLE;
				swap_chain->views[i] = VK_NULL_HANDLE;
			}

			vkDestroySwapchainKHR(context->getDevice(), swap_chain->swap_chain, nullptr);
			swap_chain->swap_chain = VK_NULL_HANDLE;
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
		Multisample samples,
		const void *data,
		uint32_t num_data_mipmaps
	)
	{
		assert(width != 0 && height != 0 && "Invalid texture size");
		assert(num_mipmaps != 0 && "Invalid mipmap count");
		assert((data == nullptr) || (data != nullptr && num_data_mipmaps != 0) && "Invalid data mipmaps");
		assert((samples == Multisample::COUNT_1) || (samples != Multisample::COUNT_1 && num_mipmaps == 1));

		vulkan::Texture *result = new vulkan::Texture();
		result->type = VK_IMAGE_TYPE_2D;
		result->format = vulkan::toFormat(format);
		result->width = width;
		result->height = height;
		result->depth = 1;
		result->num_mipmaps = num_mipmaps;
		result->num_layers = 1;
		result->samples = vulkan::toSamples(samples);
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
		uint8_t num_attachments,
		const FrameBufferAttachment *attachments
	)
	{
		// TODO: check for equal sizes (color + depthstencil)

		vulkan::FrameBuffer *result = new vulkan::FrameBuffer();

		uint32_t width = 0;
		uint32_t height = 0;
		VulkanRenderPassBuilder builder = VulkanRenderPassBuilder(context);

		builder.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS);

		// add color attachments
		result->num_attachments = 0;
		for (uint8_t i = 0; i < num_attachments; ++i)
		{
			const FrameBufferAttachment &attachment = attachments[i];
			VkImageView view = VK_NULL_HANDLE;

			if (attachment.type == FrameBufferAttachmentType::COLOR)
			{
				const FrameBufferAttachment::Color &color = attachment.color;
				const vulkan::Texture *color_texture = static_cast<const vulkan::Texture *>(color.texture);
				VkImageAspectFlags flags = vulkan::toImageAspectFlags(color_texture->format);

				view = VulkanUtils::createImageView(
					context,
					color_texture->image, color_texture->format,
					flags, VK_IMAGE_VIEW_TYPE_2D,
					color.base_mip, color.num_mips,
					color.base_layer, color.num_layers
				);

				width = std::max<int>(1, color_texture->width / (1 << color.base_mip));
				height = std::max<int>(1, color_texture->height / (1 << color.base_mip));

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
				const vulkan::Texture *depth_texture = static_cast<const vulkan::Texture *>(depth.texture);
				VkImageAspectFlags flags = vulkan::toImageAspectFlags(depth_texture->format);

				view = VulkanUtils::createImageView(
					context,
					depth_texture->image, depth_texture->format,
					flags, VK_IMAGE_VIEW_TYPE_2D
				);

				width = depth_texture->width;
				height = depth_texture->height;

				builder.addDepthStencilAttachment(depth_texture->format, depth_texture->samples);
				builder.setDepthStencilAttachmentReference(0, i);
			}
			else if (attachment.type == FrameBufferAttachmentType::SWAP_CHAIN_COLOR)
			{
				const FrameBufferAttachment::SwapChainColor &swap_chain_color = attachment.swap_chain_color;
				const vulkan::SwapChain *swap_chain = static_cast<const vulkan::SwapChain *>(swap_chain_color.swap_chain);
				VkImageAspectFlags flags = vulkan::toImageAspectFlags(swap_chain->surface_format.format);

				view = VulkanUtils::createImageView(
					context,
					swap_chain->images[swap_chain_color.base_image], swap_chain->surface_format.format,
					flags, VK_IMAGE_VIEW_TYPE_2D
				);

				width = swap_chain->sizes.width;
				height = swap_chain->sizes.height;

				if (swap_chain_color.resolve_attachment)
				{
					builder.addColorResolveAttachment(swap_chain->surface_format.format);
					builder.addColorResolveAttachmentReference(0, i);
				}
				else
				{
					builder.addColorAttachment(swap_chain->surface_format.format, VK_SAMPLE_COUNT_1_BIT);
					builder.addColorAttachmentReference(0, i);
				}
			}

			result->attachments[result->num_attachments++] = view;
		}

		// create dummy renderpass
		result->dummy_render_pass = builder.build(); // TODO: move to render pass cache

		// create framebuffer
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = result->dummy_render_pass;
		framebufferInfo.attachmentCount = result->num_attachments;
		framebufferInfo.pAttachments = result->attachments;
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

	SwapChain *VulkanDriver::createSwapChain(
		void *native_window,
		uint32_t width,
		uint32_t height
	)
	{
		assert(native_window != nullptr && "Invalid window");

		vulkan::SwapChain *result = new vulkan::SwapChain();

		// Create platform surface
		result->surface = vulkan::Platform::createSurface(context, native_window);
		if (result->surface == VK_NULL_HANDLE)
		{
			std::cerr << "VulkanDriver::createSwapChain(): can't create platform surface" << std::endl;
			destroySwapChain(result);
			return false;
		}

		// Fetch present queue family
		result->present_queue_family = VulkanUtils::getPresentQueueFamily(
			context->getPhysicalDevice(),
			result->surface,
			context->getGraphicsQueueFamily()
		);

		// Get present queue
		vkGetDeviceQueue(context->getDevice(), result->present_queue_family, 0, &result->present_queue);
		if (result->present_queue == VK_NULL_HANDLE)
		{
			std::cerr << "VulkanDriver::createSwapChain(): can't get present queue from logical device" << std::endl;
			destroySwapChain(result);
			return false;
		}

		vulkan::selectOptimalSwapChainSettings(context, result, width, height);

		// Create persistent per-frame objects
		for (size_t i = 0; i < result->num_images; i++)
		{
			// semaphores
			VkSemaphoreCreateInfo semaphore_info = {};
			semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			if (vkCreateSemaphore(context->getDevice(), &semaphore_info, nullptr, &result->image_available_gpu[i]) != VK_SUCCESS)
			{
				std::cerr << "VulkanDriver::createSwapChain(): can't create 'image available' semaphore" << std::endl;
				destroySwapChain(result);
				return nullptr;
			}

			if (vkCreateSemaphore(context->getDevice(), &semaphore_info, nullptr, &result->rendering_finished_gpu[i]) != VK_SUCCESS)
			{
				std::cerr << "VulkanDriver::createSwapChain(): can't create 'rendering finished' semaphore" << std::endl;
				destroySwapChain(result);
				return nullptr;
			}

			// fences
			VkFenceCreateInfo fence_info = {};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			if (vkCreateFence(context->getDevice(), &fence_info, nullptr, &result->rendering_finished_cpu[i]) != VK_SUCCESS)
			{
				std::cerr << "VulkanDriver::createSwapChain(): can't create 'rendering finished' fence" << std::endl;
				destroySwapChain(result);
				return nullptr;
			}
		}

		vulkan::createTransientSwapChainObjects(context, result);

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

		for (uint8_t i = 0; i < vk_frame_buffer->num_attachments; ++i)
		{
			vkDestroyImageView(context->getDevice(), vk_frame_buffer->attachments[i], nullptr);
			vk_frame_buffer->attachments[i] = VK_NULL_HANDLE;
		}

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

	void VulkanDriver::destroySwapChain(SwapChain *swap_chain)
	{
		if (swap_chain == nullptr)
			return;

		vulkan::SwapChain *vk_swap_chain = static_cast<vulkan::SwapChain *>(swap_chain);

		// Destroy persistent per-frame objects
		for (size_t i = 0; i < vk_swap_chain->num_images; i++)
		{
			vkDestroySemaphore(context->getDevice(), vk_swap_chain->image_available_gpu[i], nullptr);
			vk_swap_chain->image_available_gpu[i] = nullptr;

			vkDestroySemaphore(context->getDevice(), vk_swap_chain->rendering_finished_gpu[i], nullptr);
			vk_swap_chain->rendering_finished_gpu[i] = nullptr;

			vkDestroyFence(context->getDevice(), vk_swap_chain->rendering_finished_cpu[i], nullptr);
			vk_swap_chain->rendering_finished_cpu[i] = nullptr;
		}

		vulkan::destroyTransientSwapChainObjects(context, vk_swap_chain);

		vk_swap_chain->present_queue_family = 0xFFFF;
		vk_swap_chain->present_queue = VK_NULL_HANDLE;
		vk_swap_chain->present_mode = VK_PRESENT_MODE_FIFO_KHR;
		vk_swap_chain->sizes = {0, 0};

		vk_swap_chain->num_images = 0;
		vk_swap_chain->current_image = 0;

		// Destroy platform surface
		vulkan::Platform::destroySurface(context, vk_swap_chain->surface);
		vk_swap_chain->surface = nullptr;

		delete swap_chain;
		swap_chain = nullptr;
	}

	Multisample VulkanDriver::getMaxSampleCount()
	{
		assert(context != nullptr && "Invalid context");

		VkSampleCountFlagBits samples = context->getMaxSampleCount();
		return vulkan::fromSamples(samples);
	}

	Format VulkanDriver::getOptimalDepthFormat()
	{
		assert(context != nullptr && "Invalid context");

		VkFormat format = VulkanUtils::selectOptimalDepthFormat(context->getPhysicalDevice());
		return vulkan::fromFormat(format);
	}

	VkSampleCountFlagBits VulkanDriver::toMultisample(Multisample samples)
	{
		return vulkan::toSamples(samples);
	}

	Multisample VulkanDriver::fromMultisample(VkSampleCountFlagBits samples)
	{
		return vulkan::fromSamples(samples);
	}

	VkFormat VulkanDriver::toFormat(Format format)
	{
		return vulkan::toFormat(format);
	}

	Format VulkanDriver::fromFormat(VkFormat format)
	{
		return vulkan::fromFormat(format);
	}

	void VulkanDriver::generateTexture2DMipmaps(Texture *texture)
	{
		assert(texture != nullptr && "Invalid texture");

		vulkan::Texture *vk_texture = static_cast<vulkan::Texture *>(texture);

		// prepare for transfer
		VulkanUtils::transitionImageLayout(
			context,
			vk_texture->image,
			vk_texture->format,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0,
			vk_texture->num_mipmaps
		);

		// generate 2D mipmaps with linear filter
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

		// prepare for shader access
		VulkanUtils::transitionImageLayout(
			context,
			vk_texture->image,
			vk_texture->format,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			0,
			vk_texture->num_mipmaps
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

	bool VulkanDriver::acquire(SwapChain *swap_chain, uint32_t *new_image)
	{
		vulkan::SwapChain *vk_swap_chain = static_cast<vulkan::SwapChain *>(swap_chain);
		uint32_t current_image = vk_swap_chain->current_image;

		vkWaitForFences(
			context->getDevice(),
			1, &vk_swap_chain->rendering_finished_cpu[current_image],
			VK_TRUE, std::numeric_limits<uint64_t>::max()
		);

		VkResult result = vkAcquireNextImageKHR(
			context->getDevice(),
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

	bool VulkanDriver::present(SwapChain *swap_chain)
	{
		vulkan::SwapChain *vk_swap_chain = static_cast<vulkan::SwapChain *>(swap_chain);
		uint32_t current_image = vk_swap_chain->current_image;

		VkPresentInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &vk_swap_chain->rendering_finished_gpu[current_image]; // TODO: check if we need to wait for this
		info.swapchainCount = 1;
		info.pSwapchains = &vk_swap_chain->swap_chain;
		info.pImageIndices = &current_image;

		VulkanUtils::transitionImageLayout(
			context,
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

		vk_swap_chain->current_image++;
		vk_swap_chain->current_image %= vk_swap_chain->num_images;

		return true;
	}

	bool VulkanDriver::resize(SwapChain *swap_chain, uint32_t width, uint32_t height)
	{
		// TODO: swap chain
		return false;
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
