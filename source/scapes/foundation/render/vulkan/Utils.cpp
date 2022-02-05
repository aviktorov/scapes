#include "render/vulkan/Utils.h"
#include "render/vulkan/Context.h"

#include <cassert>
#include <algorithm>
#include <iostream>

namespace scapes::foundation::render::vulkan
{
	/*
	 */
	VkTransformMatrixKHR Utils::getTransformMatrix(
		const float col_major_matrix[16]
	)
	{
		VkTransformMatrixKHR result = {};

		result.matrix[0][0] = col_major_matrix[0];
		result.matrix[0][1] = col_major_matrix[4];
		result.matrix[0][2] = col_major_matrix[8];
		result.matrix[0][3] = col_major_matrix[12];
		result.matrix[1][0] = col_major_matrix[1];
		result.matrix[1][1] = col_major_matrix[5];
		result.matrix[1][2] = col_major_matrix[9];
		result.matrix[1][3] = col_major_matrix[13];
		result.matrix[2][0] = col_major_matrix[2];
		result.matrix[2][1] = col_major_matrix[6];
		result.matrix[2][2] = col_major_matrix[10];
		result.matrix[2][3] = col_major_matrix[14];

		return result;
	}

	/*
	 */
	VkSamplerAddressMode Utils::getSamplerAddressMode(
		SamplerWrapMode mode
	)
	{
		static VkSamplerAddressMode supported_wrap_modes[static_cast<int>(SamplerWrapMode::MAX)] =
		{
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		};

		return supported_wrap_modes[static_cast<int>(mode)];
	}

	/*
	 */
	VkFormat Utils::getFormat(Format format)
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
			VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
		};

		return supported_formats[static_cast<int>(format)];
	}

	Format Utils::getApiFormat(VkFormat format)
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
			case VK_FORMAT_X8_D24_UNORM_PACK32: return Format::D24_UNORM;
			case VK_FORMAT_D24_UNORM_S8_UINT: return Format::D24_UNORM_S8_UINT;
			case VK_FORMAT_D32_SFLOAT: return Format::D32_SFLOAT;
			case VK_FORMAT_D32_SFLOAT_S8_UINT: return Format::D32_SFLOAT_S8_UINT;

			default:
			{
				std::cerr << "vulkan::Utils::getApiFormat(): unsupported format " << format << std::endl;
				return Format::UNDEFINED;
			}
		}
	}

	/*
	 */
	VkSampleCountFlagBits Utils::getSamples(Multisample samples)
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

	Multisample Utils::getApiSamples(VkSampleCountFlagBits samples)
	{
		if (samples & VK_SAMPLE_COUNT_64_BIT) { return Multisample::COUNT_64; }
		if (samples & VK_SAMPLE_COUNT_32_BIT) { return Multisample::COUNT_32; }
		if (samples & VK_SAMPLE_COUNT_16_BIT) { return Multisample::COUNT_16; }
		if (samples & VK_SAMPLE_COUNT_8_BIT) { return Multisample::COUNT_8; }
		if (samples & VK_SAMPLE_COUNT_4_BIT) { return Multisample::COUNT_4; }
		if (samples & VK_SAMPLE_COUNT_2_BIT) { return Multisample::COUNT_2; }

		return Multisample::COUNT_1;
	}

	/*
	*/
	VkImageUsageFlags Utils::getImageUsageFlags(VkFormat format)
	{
		if (format == VK_FORMAT_UNDEFINED)
			return 0;

		switch (format)
		{
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_D16_UNORM_S8_UINT:
			case VK_FORMAT_X8_D24_UNORM_PACK32:
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT: return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}

		return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	VkImageAspectFlags Utils::getImageAspectFlags(VkFormat format)
	{
		if (format == VK_FORMAT_UNDEFINED)
			return 0;

		switch (format)
		{
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_X8_D24_UNORM_PACK32:
			case VK_FORMAT_D32_SFLOAT: return VK_IMAGE_ASPECT_DEPTH_BIT;
			case VK_FORMAT_D16_UNORM_S8_UINT:
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		return VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkImageViewType Utils::getImageBaseViewType(VkImageType type, VkImageCreateFlags flags, uint32_t num_layers)
	{
		if ((type == VK_IMAGE_TYPE_2D) && (num_layers == 1) && (flags == 0))
			return VK_IMAGE_VIEW_TYPE_2D;

		if ((type == VK_IMAGE_TYPE_2D) && (num_layers > 1) && (flags == 0))
			return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

		if ((type == VK_IMAGE_TYPE_3D) && (num_layers == 1) && (flags == 0))
			return VK_IMAGE_VIEW_TYPE_3D;

		if (type == VK_IMAGE_TYPE_2D && (num_layers == 1) && (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT))
			return VK_IMAGE_VIEW_TYPE_2D;

		if (type == VK_IMAGE_TYPE_2D && num_layers == 6 && (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT))
			return VK_IMAGE_VIEW_TYPE_CUBE;

		return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	}

	uint8_t Utils::getPixelSize(Format format)
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
			2, 3, 4, 4, 4, 5,
		};

		return supported_formats[static_cast<int>(format)];
	}

	/*
	 */
	VkIndexType Utils::getIndexType(IndexFormat format)
	{
		static VkIndexType supported_formats[static_cast<int>(IndexFormat::MAX)] =
		{
			VK_INDEX_TYPE_UINT16, VK_INDEX_TYPE_UINT32,
		};

		return supported_formats[static_cast<int>(format)];
	}

	uint8_t Utils::getIndexSize(IndexFormat format)
	{
		static uint8_t supported_formats[static_cast<int>(IndexFormat::MAX)] =
		{
			2, 4
		};

		return supported_formats[static_cast<int>(format)];
	}

	uint8_t Utils::getVertexSize(Format format)
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
			0, 0, 0, 0, 0, 0,
		};

		return supported_formats[static_cast<int>(format)];
	}

	/*
	 */
	VkPrimitiveTopology Utils::getPrimitiveTopology(RenderPrimitiveType type)
	{
		static VkPrimitiveTopology supported_types[static_cast<int>(RenderPrimitiveType::MAX)] =
		{
			// points
			VK_PRIMITIVE_TOPOLOGY_POINT_LIST,

			// lines
			VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,

			// triangles
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,

			// quads
			VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		};

		return supported_types[static_cast<int>(type)];
	}

	/*
	 */
	VkCommandBufferLevel Utils::getCommandBufferLevel(CommandBufferType type)
	{
		switch (type)
		{
			case CommandBufferType::PRIMARY: return VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			case CommandBufferType::SECONDARY: return VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			default:
			{
				std::cerr << "vulkan::Utils::getCommandBufferLevel(): unsupported command buffer type" << std::endl;
				// TODO: Log fatal?
				return VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			}
		}
	}

	/*
	 */
	VkCullModeFlags Utils::getCullMode(CullMode mode)
	{
		static VkCullModeFlags supported_cull_modes[static_cast<int>(CullMode::MAX)] =
		{
			VK_CULL_MODE_NONE,
			VK_CULL_MODE_FRONT_BIT,
			VK_CULL_MODE_BACK_BIT,
			VK_CULL_MODE_FRONT_AND_BACK,
		};

		return supported_cull_modes[static_cast<int>(mode)];
	}

	/*
	 */
	VkCompareOp Utils::getDepthCompareFunc(DepthCompareFunc func)
	{
		static VkCompareOp supported_depth_compare_funcs[static_cast<int>(DepthCompareFunc::MAX)] =
		{
			VK_COMPARE_OP_NEVER,
			VK_COMPARE_OP_LESS,
			VK_COMPARE_OP_EQUAL,
			VK_COMPARE_OP_LESS_OR_EQUAL,
			VK_COMPARE_OP_GREATER,
			VK_COMPARE_OP_NOT_EQUAL,
			VK_COMPARE_OP_GREATER_OR_EQUAL,
			VK_COMPARE_OP_ALWAYS,
		};

		return supported_depth_compare_funcs[static_cast<int>(func)];
	}

	/*
	 */
	VkBlendFactor Utils::getBlendFactor(BlendFactor factor)
	{
		static VkBlendFactor supported_blend_factors[static_cast<int>(BlendFactor::MAX)] =
		{
			VK_BLEND_FACTOR_ZERO,
			VK_BLEND_FACTOR_ONE,
			VK_BLEND_FACTOR_SRC_COLOR,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
			VK_BLEND_FACTOR_DST_COLOR,
			VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
			VK_BLEND_FACTOR_SRC_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			VK_BLEND_FACTOR_DST_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
		};

		return supported_blend_factors[static_cast<int>(factor)];
	}

	/*
	 */
	VkAttachmentLoadOp Utils::getLoadOp(RenderPassLoadOp op)
	{
		static VkAttachmentLoadOp supported_load_ops[static_cast<int>(RenderPassLoadOp::MAX)] =
		{
			VK_ATTACHMENT_LOAD_OP_LOAD,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		};

		return supported_load_ops[static_cast<int>(op)];
	}

	VkAttachmentStoreOp Utils::getStoreOp(RenderPassStoreOp op)
	{
		static VkAttachmentStoreOp supported_store_ops[static_cast<int>(RenderPassStoreOp::MAX)] =
		{
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
		};

		return supported_store_ops[static_cast<int>(op)];
	}

	/*
	 */
	bool Utils::checkInstanceValidationLayers(
		const std::vector<const char *> &requiredLayers,
		bool verbose
	)
	{
		uint32_t availableLayerCount = 0;
		vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(availableLayerCount);
		vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

		for (const char *requiredLayer : requiredLayers)
		{
			bool supported = false;
			for (const VkLayerProperties &layer : availableLayers)
			{
				if (strcmp(requiredLayer, layer.layerName) == 0)
				{
					supported = true;
					break;
				}
			}

			if (!supported)
			{
				if (verbose)
					std::cerr << requiredLayer << " is not supported" << std::endl;

				return false;
			}
			
			if (verbose)
				std::cout << "Have " << requiredLayer << std::endl;
		}

		return true;
	}

	bool Utils::checkInstanceExtensions(
		const std::vector<const char *> &requiredExtensions,
		bool verbose
	)
	{
		uint32_t availableExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

		bool result = true;
		for (const char *requiredExtension : requiredExtensions)
		{
			bool supported = false;
			for (const VkExtensionProperties &availableExtension : availableExtensions)
			{
				if (strcmp(requiredExtension, availableExtension.extensionName) == 0)
				{
					supported = true;
					break;
				}
			}

			if (!supported)
			{
				if (verbose)
					std::cerr << requiredExtension << " is not supported" << std::endl;

				result = false;
			}
			
			else if (verbose)
				std::cout << "Have " << requiredExtension << std::endl;
		}

		return result;
	}

	bool Utils::checkPhysicalDeviceExtensions(
		VkPhysicalDevice physicalDevice,
		const std::vector<const char *> &requiredExtensions,
		bool verbose
	)
	{
		uint32_t availableDeviceExtensionCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableDeviceExtensionCount, nullptr);

		std::vector<VkExtensionProperties> availableDeviceExtensions(availableDeviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableDeviceExtensionCount, availableDeviceExtensions.data());

		bool result = true;
		for (const char *requiredExtension : requiredExtensions)
		{
			bool supported = false;
			for (const VkExtensionProperties &availableDeviceExtension : availableDeviceExtensions)
			{
				if (strcmp(requiredExtension, availableDeviceExtension.extensionName) == 0)
				{
					supported = true;
					break;
				}
			}

			if (!supported)
			{
				if (verbose)
					std::cerr << requiredExtension << " is not supported on this physical device" << std::endl;

				result = false;
			}

			else if (verbose)
				std::cout << "Have " << requiredExtension << std::endl;
		}

		return result;
	}

	/*
	 */
	VkSampleCountFlagBits Utils::getMaxUsableSampleCount(
		VkPhysicalDevice physicalDevice
	)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		// TODO: this might return wrong value in case color & depth have different multisampling options
		VkSampleCountFlags counts = std::min(
			physicalDeviceProperties.limits.framebufferColorSampleCounts,
			physicalDeviceProperties.limits.framebufferDepthSampleCounts
		);

		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR Utils::getRayTracingPipelineProperties(
		VkPhysicalDevice physicalDevice
	)
	{
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracing_properties = {};
		raytracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

		VkPhysicalDeviceProperties2 properties;
		properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		properties.pNext = &raytracing_properties;

		vkGetPhysicalDeviceProperties2(physicalDevice, &properties);

		return raytracing_properties;
	}

	/*
	 */
	VkFormat Utils::selectOptimalImageFormat(
		VkPhysicalDevice physicalDevice,
		const std::vector<VkFormat> &candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features
	)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
				return format;
			
			if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
				return format;
		}

		return VK_FORMAT_UNDEFINED;
	}

	VkFormat Utils::selectOptimalDepthFormat(
		VkPhysicalDevice physicalDevice
	)
	{
		return selectOptimalImageFormat(
			physicalDevice,
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_X8_D24_UNORM_PACK32 },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	/*
	 */
	uint32_t Utils::getGraphicsQueueFamily(
		VkPhysicalDevice physicalDevice
	)
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			const auto &queueFamily = queueFamilies[i];
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				return i;
		}

		return 0xFFFF;
	}

	uint32_t Utils::getPresentQueueFamily(
		VkPhysicalDevice physicalDevice,
		VkSurfaceKHR surface,
		uint32_t graphicsQueueFamily
	)
	{
		VkBool32 presentSupported = VK_FALSE;

		if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsQueueFamily, surface, &presentSupported) != VK_SUCCESS)
			throw std::runtime_error("Can't check surface present support");

		if (presentSupported)
			return graphicsQueueFamily;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupported) != VK_SUCCESS)
				throw std::runtime_error("Can't check surface present support");

			if (presentSupported)
				return i;
		}

		return 0xFFFF;
	}

	/*
	 */
	VkDeviceAddress Utils::getBufferDeviceAddress(
		const Context *context,
		VkBuffer buffer
	)
	{
		VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
		buffer_device_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		buffer_device_address_info.buffer = buffer;
		return vkGetBufferDeviceAddress(context->getDevice(), &buffer_device_address_info);
	}

	/*
	 */
	void Utils::createBuffer(
		const Context *context,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VmaMemoryUsage memory_usage,
		VkBuffer &buffer,
		VmaAllocation &memory
	)
	{
		// Create buffer
		VkBufferCreateInfo buffer_info = {};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo alloc_info = {};
		alloc_info.usage = memory_usage;

		if (vmaCreateBuffer(context->getVRAMAllocator(), &buffer_info, &alloc_info, &buffer, &memory, nullptr) != VK_SUCCESS)
		{
			// TODO: log error "Can't create buffer"
		}
	}

	void Utils::fillDeviceLocalBuffer(
		const Context *context,
		VkBuffer buffer,
		VkDeviceSize size,
		const void *data
	)
	{
		// Create staging buffer
		VkBuffer staging_buffer = VK_NULL_HANDLE;
		VmaAllocation staging_memory = VK_NULL_HANDLE;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		if (vmaCreateBuffer(context->getVRAMAllocator(), &bufferInfo, &allocInfo, &staging_buffer, &staging_memory, nullptr) != VK_SUCCESS)
		{
			// TODO: log error "Can't create staging buffer"
		}

		// Fill staging buffer
		void *staging_data = nullptr;
		vmaMapMemory(context->getVRAMAllocator(), staging_memory, &staging_data);
		memcpy(staging_data, data, static_cast<size_t>(size));
		vmaUnmapMemory(context->getVRAMAllocator(), staging_memory);

		// Transfer to GPU local memory
		VkCommandBuffer commandBuffer = beginSingleTimeCommands(context);

		VkBufferCopy copyRegion = {};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, staging_buffer, buffer, 1, &copyRegion);

		endSingleTimeCommands(context, commandBuffer);

		// Destroy staging buffer
		vmaDestroyBuffer(context->getVRAMAllocator(), staging_buffer, staging_memory);
	}

	void Utils::fillHostVisibleBuffer(
		const Context *context,
		VmaAllocation memory,
		VkDeviceSize size,
		const void *data
	)
	{
		// Fill buffer
		void *buffer_data = nullptr;
		vmaMapMemory(context->getVRAMAllocator(), memory, &buffer_data);
		memcpy(buffer_data, data, static_cast<size_t>(size));
		vmaUnmapMemory(context->getVRAMAllocator(), memory);
	}

	/*
	 */
	VkShaderModule Utils::createShaderModule(
		const Context *context,
		const uint32_t *data,
		size_t size
	)
	{
		VkShaderModuleCreateInfo shaderInfo = {};
		shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderInfo.codeSize = size;
		shaderInfo.pCode = data;

		VkShaderModule shader = VK_NULL_HANDLE;
		if (vkCreateShaderModule(context->getDevice(), &shaderInfo, nullptr, &shader) != VK_SUCCESS)
			return VK_NULL_HANDLE;

		return shader;
	}

	/*
	 */
	void Utils::createImage(
		const Context *context,
		VkImageType type,
		uint32_t width,
		uint32_t height,
		uint32_t depth,
		uint32_t mipLevels,
		uint32_t arrayLayers,
		VkSampleCountFlagBits numSamples,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkImageCreateFlags flags,
		VkImage &image,
		VmaAllocation &memory
	)
	{
		// Create buffer
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = type;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = depth;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = arrayLayers;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.usage = usage;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = numSamples;
		imageInfo.flags = flags;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VkResult result = vmaCreateImage(context->getVRAMAllocator(), &imageInfo, &allocInfo, &image, &memory, nullptr);
		assert((result == VK_SUCCESS) && "Can't create image");
	}

	VkImageView Utils::createImageView(
		const Context *context,
		const Texture *texture,
		uint32_t base_mip,
		uint32_t num_mips,
		uint32_t base_layer,
		uint32_t num_layers
	)
	{
		VkImageAspectFlags aspect_flags = Utils::getImageAspectFlags(texture->format);
		VkImageViewType view_type = Utils::getImageBaseViewType(texture->type, texture->flags, num_layers);

		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = texture->image;
		info.viewType = view_type;
		info.format = texture->format;
		info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		info.subresourceRange.aspectMask = aspect_flags;
		info.subresourceRange.baseMipLevel = base_mip;
		info.subresourceRange.levelCount = num_mips;
		info.subresourceRange.baseArrayLayer = base_layer;
		info.subresourceRange.layerCount = num_layers;

		VkImageView result = VK_NULL_HANDLE;
		if (vkCreateImageView(context->getDevice(), &info, nullptr, &result) != VK_SUCCESS)
			return VK_NULL_HANDLE;

		return result;
	}

	VkSampler Utils::createSampler(
		const Context *context,
		uint32_t minMipLevel,
		uint32_t maxMipLevel,
		VkSamplerAddressMode addressModeU,
		VkSamplerAddressMode addressModeV,
		VkSamplerAddressMode addressModeW,
		bool depthCompareEnabled,
		VkCompareOp depthCompareFunc
	)
	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = addressModeU;
		samplerInfo.addressModeV = addressModeV;
		samplerInfo.addressModeW = addressModeW;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = depthCompareEnabled;
		samplerInfo.compareOp = depthCompareFunc;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = static_cast<float>(minMipLevel);
		samplerInfo.maxLod = static_cast<float>(maxMipLevel);

		VkSampler sampler = VK_NULL_HANDLE;
		if (vkCreateSampler(context->getDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
			return VK_NULL_HANDLE;

		return sampler;
	}

	/*
	 */
	bool Utils::createAccelerationStructure(
		const Context *context,
		VkAccelerationStructureTypeKHR type,
		VkBuildAccelerationStructureFlagBitsKHR build_flags,
		uint32_t num_geometries,
		const VkAccelerationStructureGeometryKHR *geometries,
		const uint32_t *max_primitives,
		AccelerationStructure *result
	)
	{
		VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
		build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		build_info.type = type;
		build_info.flags = build_flags;
		build_info.geometryCount = num_geometries;
		build_info.pGeometries = geometries;

		VkAccelerationStructureBuildSizesInfoKHR build_sizes_info = {};
		build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		vkGetAccelerationStructureBuildSizesKHR(
			context->getDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&build_info,
			max_primitives,
			&build_sizes_info
		);

		result->size = build_sizes_info.accelerationStructureSize;
		result->update_scratch_size = build_sizes_info.updateScratchSize;
		result->build_scratch_size = build_sizes_info.buildScratchSize;

		VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;

		Utils::createBuffer(context, result->size, usage_flags, memory_usage, result->buffer, result->memory);

		VkAccelerationStructureCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		create_info.buffer = result->buffer;
		create_info.size = result->size;
		create_info.type = type;

		if (vkCreateAccelerationStructureKHR(context->getDevice(), &create_info, nullptr, &result->acceleration_structure) != VK_SUCCESS)
			return false;

		return true;
	}

	void Utils::buildAccelerationStructure(
		const Context *context,
		VkAccelerationStructureTypeKHR type,
		VkBuildAccelerationStructureFlagBitsKHR build_flags,
		uint32_t num_geometries,
		const VkAccelerationStructureGeometryKHR *geometries,
		const VkAccelerationStructureBuildRangeInfoKHR *ranges,
		AccelerationStructure *result
	)
	{
		VkBufferUsageFlags scratch_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		VmaMemoryUsage scratch_memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VkBuffer scratch_buffer = VK_NULL_HANDLE;
		VmaAllocation scratch_memory = VK_NULL_HANDLE;

		Utils::createBuffer(context, result->build_scratch_size, scratch_usage_flags, scratch_memory_usage, scratch_buffer, scratch_memory);

		VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
		build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		build_info.type = type;
		build_info.flags = build_flags;
		build_info.geometryCount = num_geometries;
		build_info.pGeometries = geometries;
		build_info.dstAccelerationStructure = result->acceleration_structure;
		build_info.scratchData.deviceAddress = Utils::getBufferDeviceAddress(context, scratch_buffer);

		const VkAccelerationStructureBuildRangeInfoKHR *build_ranges[] = { ranges };

		VkCommandBuffer command_buffer = Utils::beginSingleTimeCommands(context);

		vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &build_info, build_ranges);

		Utils::endSingleTimeCommands(context, command_buffer);

		vmaDestroyBuffer(context->getVRAMAllocator(), scratch_buffer, scratch_memory);
	}

	/*
	 */
	void Utils::fillImage(
		const Context *context,
		VkImage image,
		uint32_t width,
		uint32_t height,
		uint32_t depth,
		uint32_t mipLevels,
		uint32_t arrayLayers,
		uint32_t pixelSize,
		VkFormat format,
		const void *data,
		uint32_t dataMipLevels,
		uint32_t dataArrayLayers
	)
	{
		VkDeviceSize resource_size = 0;
		uint32_t mip_width = width;
		uint32_t mip_height = height;
		uint32_t mip_depth = depth;

		for (uint32_t i = 0; i < dataMipLevels; i++)
		{
			resource_size += mip_width * mip_height * pixelSize;
			mip_width = std::max<int>(mip_width / 2, 1);
			mip_height = std::max<int>(mip_height / 2, 1);
			mip_depth = std::max<int>(mip_depth / 2, 1);
		}

		// Create staging buffer
		VkBuffer staging_buffer = VK_NULL_HANDLE;
		VmaAllocation staging_memory = VK_NULL_HANDLE;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = resource_size * dataArrayLayers;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		if (vmaCreateBuffer(context->getVRAMAllocator(), &bufferInfo, &allocInfo, &staging_buffer, &staging_memory, nullptr) != VK_SUCCESS)
		{
			// TODO: log error "Can't create staging buffer"
		}

		// Fill staging buffer
		void *staging_data = nullptr;
		vmaMapMemory(context->getVRAMAllocator(), staging_memory, &staging_data);
		memcpy(staging_data, data, static_cast<size_t>(resource_size * dataArrayLayers));
		vmaUnmapMemory(context->getVRAMAllocator(), staging_memory);

		// Copy to the image memory on GPU
		VkDeviceSize offset = 0;

		VkCommandBuffer command_buffer = beginSingleTimeCommands(context);

		for (uint32_t i = 0; i < dataArrayLayers; i++)
		{
			mip_width = width;
			mip_height = height;
			mip_depth = depth;

			for (uint32_t j = 0; j < dataMipLevels; j++)
			{
				VkBufferImageCopy region = {};
				region.bufferOffset = offset;
				region.bufferRowLength = 0;
				region.bufferImageHeight = 0;

				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = j;
				region.imageSubresource.baseArrayLayer = i;
				region.imageSubresource.layerCount = 1;

				region.imageOffset = {0, 0, 0};
				region.imageExtent.width = width;
				region.imageExtent.height = height;
				region.imageExtent.depth = 1;

				vkCmdCopyBufferToImage(command_buffer, staging_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

				offset += mip_width * mip_height * mip_depth * pixelSize;
				mip_width /= 2;
				mip_height /= 2;
			}
		}

		Utils::endSingleTimeCommands(context, command_buffer);

		// Destroy staging buffer
		vmaDestroyBuffer(context->getVRAMAllocator(), staging_buffer, staging_memory);
	}

	void Utils::generateImage2DMipmaps(
		const Context *context,
		VkImage image,
		VkFormat imageFormat,
		uint32_t width,
		uint32_t height,
		uint32_t mipLevels,
		VkFormat format,
		VkFilter filter
	)
	{
		if (mipLevels == 1)
			return;

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(context->getPhysicalDevice(), format, &formatProperties);

		bool supportsLinearFiltering = (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
		bool supportsCubicFiltering = (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_EXT) != 0;

		if (filter == VK_FILTER_LINEAR && !supportsLinearFiltering)
			throw std::runtime_error("Linear filtering is not supported on this device");

		if (filter == VK_FILTER_CUBIC_EXT && !supportsCubicFiltering)
			throw std::runtime_error("Cubic filtering is not supported on this device");

		// generate mips
		VkCommandBuffer commandBuffer = beginSingleTimeCommands(context);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = width;
		int32_t mipHeight = height;

		for (uint32_t i = 1; i < mipLevels; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			VkImageBlit blit = {};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { std::max(1, mipWidth / 2), std::max(1, mipHeight / 2), 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(
				commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				filter
			);

			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			mipWidth = std::max(1, mipWidth / 2);
			mipHeight = std::max(1, mipHeight / 2);
		}

		endSingleTimeCommands(context, commandBuffer);
	}

	/*
	 */
	void Utils::transitionImageLayout(
		const Context *context,
		VkImage image,
		VkFormat format,
		VkImageLayout old_layout,
		VkImageLayout new_layout,
		uint32_t base_mip,
		uint32_t num_mips,
		uint32_t base_layer,
		uint32_t num_layers
	)
	{
		struct LayoutTransition
		{
			VkImageLayout layout;
			VkAccessFlags access_mask;
			VkPipelineStageFlags stage;
		};

		static std::vector<LayoutTransition> supported_transitions =
		{
			{ VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT },
			{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT },
			{ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT },
			{ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,	VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },
			{ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
			{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,	VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT },
			{ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT },
		};

		VkCommandBuffer command_buffer = beginSingleTimeCommands(context);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = old_layout;
		barrier.newLayout = new_layout;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		barrier.subresourceRange.aspectMask = getImageAspectFlags(format);
		barrier.subresourceRange.baseMipLevel = base_mip;
		barrier.subresourceRange.levelCount = num_mips;
		barrier.subresourceRange.baseArrayLayer = base_layer;
		barrier.subresourceRange.layerCount = num_layers;

		VkPipelineStageFlags src_stage;
		VkPipelineStageFlags dst_stage;

		bool src_valid = false;
		bool dst_valid = false;

		for (size_t i = 0; i < supported_transitions.size(); ++i)
		{
			const LayoutTransition &transition = supported_transitions[i];
			if (old_layout == transition.layout)
			{
				barrier.srcAccessMask = transition.access_mask;
				src_stage = transition.stage;
				src_valid = true;
			}
			
			if (new_layout == transition.layout)
			{
				barrier.dstAccessMask = transition.access_mask;
				dst_stage = transition.stage;
				dst_valid = true;
			}
		}

		assert(src_valid && dst_valid && "Unsupported layout transition");

		vkCmdPipelineBarrier(
			command_buffer,
			src_stage, dst_stage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		endSingleTimeCommands(context, command_buffer);
	}

	/*
	 */
	VkCommandBuffer Utils::beginSingleTimeCommands(const Context *context)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = context->getCommandPool();
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(context->getDevice(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void Utils::endSingleTimeCommands(const Context *context, VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = 0;

		VkFence fence = VK_NULL_HANDLE;

		VkResult result = vkCreateFence(context->getDevice(), &fenceInfo, nullptr, &fence);
		assert((result == VK_SUCCESS) && "Can't create fence");

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		result = vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, fence);
		assert((result == VK_SUCCESS) && "Can't submit command buffer");
		
		result = vkWaitForFences(context->getDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
		assert((result == VK_SUCCESS) && "Can't wait for a fence");

		vkDestroyFence(context->getDevice(), fence, nullptr);
		vkFreeCommandBuffers(context->getDevice(), context->getCommandPool(), 1, &commandBuffer);
	}
}
