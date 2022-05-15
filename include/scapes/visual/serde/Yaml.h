#pragma once

#include <scapes/foundation/serde/Yaml.h>
#include <scapes/visual/hardware/Device.h>
#include <scapes/visual/GroupParameterType.h>

namespace c4
{
	namespace yaml = scapes::foundation::serde::yaml;
	namespace hardware = scapes::visual::hardware;

	/*
	 */
	SCAPES_INLINE bool from_chars(const yaml::csubstr buf, hardware::RenderPassClearColor *clear_color)
	{
		size_t ret = yaml::unformat(
			buf,
			"{},{},{},{}",
			clear_color->as_f32[0],
			clear_color->as_f32[1],
			clear_color->as_f32[2],
			clear_color->as_f32[3]
		);

		return ret != ryml::yml::npos;
	}

	SCAPES_INLINE size_t to_chars(yaml::substr buffer, hardware::RenderPassClearColor clear_color)
	{
		return ryml::format(
			buffer,
			"{},{},{},{}",
			clear_color.as_f32[0],
			clear_color.as_f32[1],
			clear_color.as_f32[2],
			clear_color.as_f32[3]
		);
	}

	/*
	 */
	SCAPES_INLINE bool from_chars(const yaml::csubstr buf, hardware::RenderPassClearDepthStencil *clear_depthstencil)
	{
		size_t ret = yaml::unformat(
			buf,
			"{},{}",
			clear_depthstencil->depth,
			clear_depthstencil->stencil
		);

		return ret != ryml::yml::npos;
	}

	SCAPES_INLINE size_t to_chars(yaml::substr buffer, hardware::RenderPassClearDepthStencil clear_depthstencil)
	{
		return ryml::format(
			buffer,
			"{},{}",
			clear_depthstencil.depth,
			clear_depthstencil.stencil
		);
	}

	/*
	 */
	static const char *supported_render_pass_load_ops[static_cast<size_t>(hardware::RenderPassLoadOp::MAX)] =
	{
		"LOAD",
		"CLEAR",
		"DONT_CARE",
	};

	SCAPES_INLINE bool from_chars(const yaml::csubstr buf, hardware::RenderPassLoadOp *load_op)
	{
		for (size_t i = 0; i < static_cast<size_t>(hardware::RenderPassLoadOp::MAX); ++i)
		{
			if (buf.compare(supported_render_pass_load_ops[i], strlen(supported_render_pass_load_ops[i])) == 0)
			{
				*load_op = static_cast<hardware::RenderPassLoadOp>(i);
				return true;
			}
		}

		return false;
	}

	SCAPES_INLINE size_t to_chars(yaml::substr buffer, hardware::RenderPassLoadOp load_op)
	{
		return ryml::format(buffer, "{}", supported_render_pass_load_ops[static_cast<size_t>(load_op)]);
	}

	/*
	 */
	static const char *supported_render_pass_store_ops[static_cast<size_t>(hardware::RenderPassStoreOp::MAX)] =
	{
		"STORE",
		"DONT_CARE",
	};

	SCAPES_INLINE bool from_chars(const yaml::csubstr buf, hardware::RenderPassStoreOp *store_op)
	{
		for (size_t i = 0; i < static_cast<size_t>(hardware::RenderPassStoreOp::MAX); ++i)
		{
			if (buf.compare(supported_render_pass_store_ops[i], strlen(supported_render_pass_store_ops[i])) == 0)
			{
				*store_op = static_cast<hardware::RenderPassStoreOp>(i);
				return true;
			}
		}

		return false;
	}

	SCAPES_INLINE size_t to_chars(yaml::substr buffer, hardware::RenderPassStoreOp store_op)
	{
		return ryml::format(buffer, "{}", supported_render_pass_store_ops[static_cast<size_t>(store_op)]);
	}

	/*
	 */
	static const char *supported_render_formats[static_cast<size_t>(hardware::Format::MAX)] =
	{
		"UNDEFINED",

		"R8_UNORM", "R8_SNORM", "R8_UINT", "R8_SINT",
		"R8G8_UNORM", "R8G8_SNORM", "R8G8_UINT", "R8G8_SINT",
		"R8G8B8_UNORM", "R8G8B8_SNORM", "R8G8B8_UINT", "R8G8B8_SINT",
		"B8G8R8_UNORM", "B8G8R8_SNORM", "B8G8R8_UINT", "B8G8R8_SINT",
		"R8G8B8A8_UNORM", "R8G8B8A8_SNORM", "R8G8B8A8_UINT", "R8G8B8A8_SINT",
		"B8G8R8A8_UNORM", "B8G8R8A8_SNORM", "B8G8R8A8_UINT", "B8G8R8A8_SINT",

		"R16_UNORM", "R16_SNORM", "R16_UINT", "R16_SINT", "R16_SFLOAT", "R16G16_UNORM",
		"R16G16_SNORM", "R16G16_UINT", "R16G16_SINT", "R16G16_SFLOAT",
		"R16G16B16_UNORM", "R16G16B16_SNORM", "R16G16B16_UINT", "R16G16B16_SINT", "R16G16B16_SFLOAT",
		"R16G16B16A16_UNORM", "R16G16B16A16_SNORM", "R16G16B16A16_UINT", "R16G16B16A16_SINT", "R16G16B16A16_SFLOAT",

		"R32_UINT", "R32_SINT", "R32_SFLOAT",
		"R32G32_UINT", "R32G32_SINT", "R32G32_SFLOAT",
		"R32G32B32_UINT", "R32G32B32_SINT", "R32G32B32_SFLOAT",
		"R32G32B32A32_UINT", "R32G32B32A32_SINT", "R32G32B32A32_SFLOAT",

		"D16_UNORM", "D16_UNORM_S8_UINT", "D24_UNORM", "D24_UNORM_S8_UINT", "D32_SFLOAT", "D32_SFLOAT_S8_UINT",
	};

	SCAPES_INLINE bool from_chars(const yaml::csubstr buf, hardware::Format *format)
	{
		for (size_t i = 0; i < static_cast<size_t>(hardware::Format::MAX); ++i)
		{
			if (buf.compare(supported_render_formats[i], strlen(supported_render_formats[i])) == 0)
			{
				*format = static_cast<hardware::Format>(i);
				return true;
			}
		}

		return false;
	}

	SCAPES_INLINE size_t to_chars(yaml::substr buffer, hardware::Format format)
	{
		if (format == hardware::Format::UNDEFINED)
			return 0;

		return ryml::format(buffer, "{}", supported_render_formats[static_cast<size_t>(format)]);
	}

	/*
	 */
	static const char *supported_group_parameter_types[static_cast<size_t>(scapes::visual::GroupParameterType::MAX)] =
	{
		"undefined",
		"float", "int", "uint",
		"vec2", "vec3", "vec4",
		"ivec2", "ivec3", "ivec4",
		"uvec2", "uvec3", "uvec4",
		"mat3", "mat4",
	};

	SCAPES_INLINE bool from_chars(const yaml::csubstr buf, scapes::visual::GroupParameterType *type)
	{
		// NOTE: start index is intentionally set to 1 in order to skip check for UNDEFINED type
		for (size_t i = 1; i < static_cast<size_t>(scapes::visual::GroupParameterType::MAX); ++i)
		{
			if (buf.compare(supported_group_parameter_types[i], strlen(supported_group_parameter_types[i])) == 0)
			{
				*type = static_cast<scapes::visual::GroupParameterType>(i);
				return true;
			}
		}

		return false;
	}

	SCAPES_INLINE size_t to_chars(yaml::substr buffer, scapes::visual::GroupParameterType type)
	{
		if (type == scapes::visual::GroupParameterType::UNDEFINED)
			return 0;

		return ryml::format(buffer, "{}", supported_group_parameter_types[static_cast<size_t>(type)]);
	}
}
