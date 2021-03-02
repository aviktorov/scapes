#include <common/Log.h>
#include <render/backend/opengl/Utils.h>

#include <cassert>

namespace render::backend::opengl
{

//
GLenum Utils::getInternalFormat(Format format)
{
	static GLenum internal_formats[] = 
	{
		// undefined
		GL_NONE,

		// 8-bit formats
		GL_R8, GL_R8_SNORM, GL_R8UI, GL_R8I,
		GL_RG8, GL_RG8_SNORM, GL_RG8UI, GL_RG8I,
		GL_RGB8, GL_RGB8_SNORM, GL_RGB8UI, GL_RGB8I,
		GL_NONE, GL_NONE, GL_NONE, GL_NONE,
		GL_RGBA8, GL_RGBA8_SNORM, GL_RGBA8UI, GL_RGBA8I,
		GL_NONE, GL_NONE, GL_NONE, GL_NONE,

		// 16-bit formats
		GL_R16, GL_R16_SNORM, GL_R16UI, GL_R16I, GL_R16F,
		GL_RG16, GL_RG16_SNORM, GL_RG16UI, GL_RG16I, GL_RG16F,
		GL_RGB16, GL_RGB16_SNORM, GL_RGB16UI, GL_RGB16I, GL_RGB16F,
		GL_RGBA16, GL_RGBA16_SNORM, GL_RGBA16UI, GL_RGBA16I, GL_RGBA16F,

		// 32-bit formats
		GL_R32UI, GL_R32I, GL_R32F,
		GL_RG32UI, GL_RG32I, GL_RG32F,
		GL_RGB32UI, GL_RGB32I, GL_RGB32F,
		GL_RGBA32UI, GL_RGBA32I, GL_RGBA32F,

		// depth formats
		GL_DEPTH_COMPONENT16, GL_NONE, GL_DEPTH_COMPONENT24, GL_DEPTH24_STENCIL8, GL_DEPTH_COMPONENT32F, GL_DEPTH32F_STENCIL8,
	};

	if (format == Format::D16_UNORM_S8_UINT)
		Log::warning("opengl::Utils::getInternalFormat(): D16_UNORM_S8_UINT format is not supported in OpenGL\n");

	if (format >= Format::B8G8R8_UNORM && format <= Format::B8G8R8_SINT)
		Log::warning("opengl::Utils::getInternalFormat(): BGR8 formats are not supported in OpenGL\n");

	if (format >= Format::B8G8R8A8_UNORM && format <= Format::B8G8R8A8_SINT)
		Log::warning("opengl::Utils::getInternalFormat(): BGRA8 formats are not supported in OpenGL\n");

	return internal_formats[static_cast<int>(format)];
}

GLenum Utils::getFormat(Format format)
{
	static GLenum formats[] = 
	{
		// undefined
		GL_NONE,

		// 8-bit formats
		GL_RED, GL_RED, GL_RED, GL_RED,
		GL_RG, GL_RG, GL_RG, GL_RG,
		GL_RGB, GL_RGB, GL_RGB, GL_RGB,
		GL_NONE, GL_NONE, GL_NONE, GL_NONE,
		GL_RGBA, GL_RGBA, GL_RGBA, GL_RGBA,
		GL_NONE, GL_NONE, GL_NONE, GL_NONE,

		// 16-bit formats
		GL_RED, GL_RED, GL_RED, GL_RED, GL_RED,
		GL_RG, GL_RG, GL_RG, GL_RG, GL_RG,
		GL_RGB, GL_RGB, GL_RGB, GL_RGB, GL_RGB,
		GL_RGBA, GL_RGBA, GL_RGBA, GL_RGBA, GL_RGBA,

		// 32-bit formats
		GL_RED, GL_RED, GL_RED,
		GL_RG, GL_RG, GL_RG,
		GL_RGB, GL_RGB, GL_RGB,
		GL_RGBA, GL_RGBA, GL_RGBA,

		// depth formats
		GL_DEPTH_COMPONENT, GL_NONE, GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL, GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL,
	};

	if (format == Format::D16_UNORM_S8_UINT)
		Log::warning("opengl::Utils::getFormat(): D16_UNORM_S8_UINT format is not supported in OpenGL\n");

	if (format >= Format::B8G8R8_UNORM && format <= Format::B8G8R8_SINT)
		Log::warning("opengl::Utils::getFormat(): BGR8 formats are not supported in OpenGL\n");

	if (format >= Format::B8G8R8A8_UNORM && format <= Format::B8G8R8A8_SINT)
		Log::warning("opengl::Utils::getFormat(): BGRA8 formats are not supported in OpenGL\n");

	return formats[static_cast<int>(format)];
}

GLenum Utils::getPixelType(Format format)
{
	static GLenum pixel_types[] =
	{
		// undefined
		GL_NONE,

		// 8-bit formats
		GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_BYTE, GL_BYTE,
		GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_BYTE, GL_BYTE,
		GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_BYTE, GL_BYTE,
		GL_NONE, GL_NONE, GL_NONE, GL_NONE,
		GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_BYTE, GL_BYTE,
		GL_NONE, GL_NONE, GL_NONE, GL_NONE,

		// 16-bit formats
		GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_SHORT, GL_SHORT, GL_HALF_FLOAT,
		GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_SHORT, GL_SHORT, GL_HALF_FLOAT,
		GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_SHORT, GL_SHORT, GL_HALF_FLOAT,
		GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_SHORT, GL_SHORT, GL_HALF_FLOAT,

		// 32-bit formats
		GL_UNSIGNED_INT, GL_INT, GL_FLOAT,
		GL_UNSIGNED_INT, GL_INT, GL_FLOAT,
		GL_UNSIGNED_INT, GL_INT, GL_FLOAT,
		GL_UNSIGNED_INT, GL_INT, GL_FLOAT,

		// depth formats
		GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE,
	};

	if (format >= Format::D16_UNORM && format <= Format::D32_SFLOAT_S8_UINT)
		Log::warning("opengl::Utils::getPixelType(): depth formats are not supported in OpenGL\n");

	if (format >= Format::B8G8R8_UNORM && format <= Format::B8G8R8_SINT)
		Log::warning("opengl::Utils::getPixelType(): BGR8 formats are not supported in OpenGL\n");

	if (format >= Format::B8G8R8A8_UNORM && format <= Format::B8G8R8A8_SINT)
		Log::warning("opengl::Utils::getPixelType(): BGRA8 formats are not supported in OpenGL\n");

	return pixel_types[static_cast<int>(format)];
}


GLint Utils::getPixelSize(Format format)
{
	static GLint pixel_sizes[] =
	{
		// undefined
		0,

		// 8-bit formats
		1, 1, 1, 1,
		2, 2, 2, 2,
		3, 3, 3, 3,
		0, 0, 0, 0,
		4, 4, 4, 4,
		0, 0, 0, 0,

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

	if (format >= Format::D16_UNORM && format <= Format::D32_SFLOAT_S8_UINT)
		Log::warning("opengl::Utils::getPixelSize(): depth formats are not supported in OpenGL\n");

	if (format >= Format::B8G8R8_UNORM && format <= Format::B8G8R8_SINT)
		Log::warning("opengl::Utils::getPixelSize(): BGR8 formats are not supported in OpenGL\n");

	if (format >= Format::B8G8R8A8_UNORM && format <= Format::B8G8R8A8_SINT)
		Log::warning("opengl::Utils::getPixelSize(): BGRA8 formats are not supported in OpenGL\n");

	return pixel_sizes[static_cast<int>(format)];
}

//
GLenum Utils::getFramebufferDepthStencilAttachmentType(GLenum internal_format)
{
	switch (internal_format)
	{
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32F: return GL_DEPTH_ATTACHMENT;
		case GL_DEPTH24_STENCIL8:
		case GL_DEPTH32F_STENCIL8: return GL_DEPTH_STENCIL_ATTACHMENT;
	}

	return GL_NONE;
}

//
GLint Utils::getAttributeComponents(Format format)
{
	static GLint attribute_components[] =
	{
		// undefined
		0,

		// 8-bit formats
		1, 1, 1, 1,
		2, 2, 2, 2,
		3, 3, 3, 3,
		0, 0, 0, 0,
		4, 4, 4, 4,
		0, 0, 0, 0,

		// 16-bit formats
		1, 1, 1, 1, 1,
		2, 2, 2, 2, 2,
		3, 3, 3, 3, 3,
		4, 4, 4, 4, 4,

		// 32-bit formats
		1, 1, 1,
		2, 2, 2,
		3, 3, 3,
		4, 4, 4,

		// depth formats
		0, 0, 0, 0, 0, 0,
	};

	if (format >= Format::D16_UNORM && format <= Format::D32_SFLOAT_S8_UINT)
		Log::warning("opengl::Utils::getAttributeComponents(): depth formats are not supported in OpenGL\n");

	if (format >= Format::B8G8R8_UNORM && format <= Format::B8G8R8_SINT)
		Log::warning("opengl::Utils::getAttributeComponents(): BGR8 formats are not supported in OpenGL\n");

	if (format >= Format::B8G8R8A8_UNORM && format <= Format::B8G8R8A8_SINT)
		Log::warning("opengl::Utils::getAttributeComponents(): BGRA8 formats are not supported in OpenGL\n");

	return attribute_components[static_cast<int>(format)];
}

GLenum Utils::getAttributeType(Format format)
{
	static GLenum attribute_types[] =
	{
		// undefined
		GL_NONE,

		// 8-bit formats
		GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_BYTE, GL_BYTE,
		GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_BYTE, GL_BYTE,
		GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_BYTE, GL_BYTE,
		GL_NONE, GL_NONE, GL_NONE, GL_NONE,
		GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_BYTE, GL_BYTE,
		GL_NONE, GL_NONE, GL_NONE, GL_NONE,

		// 16-bit formats
		GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_SHORT, GL_SHORT, GL_HALF_FLOAT,
		GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_SHORT, GL_SHORT, GL_HALF_FLOAT,
		GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_SHORT, GL_SHORT, GL_HALF_FLOAT,
		GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_SHORT, GL_SHORT, GL_HALF_FLOAT,

		// 32-bit formats
		GL_UNSIGNED_INT, GL_INT, GL_FLOAT,
		GL_UNSIGNED_INT, GL_INT, GL_FLOAT,
		GL_UNSIGNED_INT, GL_INT, GL_FLOAT,
		GL_UNSIGNED_INT, GL_INT, GL_FLOAT,

		// depth formats
		GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE,
	};

	if (format >= Format::D16_UNORM && format <= Format::D32_SFLOAT_S8_UINT)
		Log::warning("opengl::Utils::getAttributeType(): depth formats are not supported in OpenGL\n");

	if (format >= Format::B8G8R8_UNORM && format <= Format::B8G8R8_SINT)
		Log::warning("opengl::Utils::getAttributeType(): BGR8 formats are not supported in OpenGL\n");

	if (format >= Format::B8G8R8A8_UNORM && format <= Format::B8G8R8A8_SINT)
		Log::warning("opengl::Utils::getAttributeType(): BGRA8 formats are not supported in OpenGL\n");

	return attribute_types[static_cast<int>(format)];
}

GLboolean Utils::isAttributeNormalized(Format format)
{
	static GLboolean attribute_normalized[] =
	{
		// undefined
		GL_FALSE,

		// 8-bit formats
		GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE,
		GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE,
		GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE,
		GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE,
		GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE,
		GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE,

		// 16-bit formats
		GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE,
		GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE,
		GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE,
		GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE,

		// 32-bit formats
		GL_FALSE, GL_FALSE, GL_FALSE,
		GL_FALSE, GL_FALSE, GL_FALSE,
		GL_FALSE, GL_FALSE, GL_FALSE,
		GL_FALSE, GL_FALSE, GL_FALSE,

		// depth formats
		GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE,
	};

	if (format >= Format::D16_UNORM && format <= Format::D32_SFLOAT_S8_UINT)
		Log::warning("opengl::Utils::isAttributeNormalized(): depth formats are not supported in OpenGL\n");

	if (format >= Format::B8G8R8_UNORM && format <= Format::B8G8R8_SINT)
		Log::warning("opengl::Utils::isAttributeNormalized(): BGR8 formats are not supported in OpenGL\n");

	if (format >= Format::B8G8R8A8_UNORM && format <= Format::B8G8R8A8_SINT)
		Log::warning("opengl::Utils::isAttributeNormalized(): BGRA8 formats are not supported in OpenGL\n");

	return attribute_normalized[static_cast<int>(format)];
}

//
GLenum Utils::getIndexFormat(IndexFormat format)
{
	static GLenum index_formats[] =
	{
		GL_UNSIGNED_SHORT, GL_UNSIGNED_INT,
	};

	return index_formats[static_cast<int>(format)];
}

GLint Utils::getIndexSize(IndexFormat format)
{
	static GLint index_sizes[] =
	{
		2, 4,
	};

	return index_sizes[static_cast<int>(format)];
}

//
GLint Utils::getSampleCount(Multisample samples)
{
	static GLint sample_count[] =
	{
		1, 2, 4, 8, 16, 32, 64,
	};

	return sample_count[static_cast<int>(samples)];
}

//
Multisample Utils::getMultisample(GLint sample_count)
{
	switch (sample_count)
	{
		case 0:
		case 1: return Multisample::COUNT_1;
		case 2: return Multisample::COUNT_2;
		case 4: return Multisample::COUNT_4;
		case 8: return Multisample::COUNT_8;
		case 16: return Multisample::COUNT_16;
		case 32: return Multisample::COUNT_32;
	}

	Log::warning("opengl::Utils::getMultisample(): unsupported sample count: %d\n", sample_count);
	return Multisample::COUNT_1;
}


//
GLenum Utils::getDepthCompareFunc(DepthCompareFunc func)
{
	static GLenum depth_funcs[] =
	{
		GL_NEVER,
		GL_LESS,
		GL_EQUAL,
		GL_LEQUAL,
		GL_GREATER,
		GL_NOTEQUAL,
		GL_GEQUAL,
		GL_ALWAYS,
	};

	return depth_funcs[static_cast<int>(func)];
}

//
GLenum Utils::getBlendFactor(BlendFactor factor)
{
	static GLenum blend_factors[] =
	{
		GL_ZERO,
		GL_ONE,
		GL_SRC_COLOR,
		GL_ONE_MINUS_SRC_COLOR,
		GL_DST_COLOR,
		GL_ONE_MINUS_DST_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
	};

	return blend_factors[static_cast<int>(factor)];
}

//
GLenum Utils::getCullMode(CullMode mode)
{
	static GLenum cull_modes[] =
	{
		GL_NONE,
		GL_FRONT,
		GL_BACK,
		GL_FRONT_AND_BACK,
	};

	return cull_modes[static_cast<int>(mode)];
}

//
GLenum Utils::getPrimitiveType(RenderPrimitiveType type)
{
	static GLenum primitive_types[] =
	{
		// points
		GL_POINTS,

		// lines
		GL_LINES, GL_LINE_STRIP, GL_PATCHES,

		// triangles
		GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_PATCHES,

		// quads
		GL_PATCHES,
	};

	return primitive_types[static_cast<int>(type)];
}

GLint Utils::getPrimitivePatchComponents(RenderPrimitiveType type)
{
	static GLint primitive_patch_components[] =
	{
		// points
		0,

		// lines
		0, 0, 2,

		// triangles
		0, 0, 0, 3,

		// quads
		4,
	};

	return primitive_patch_components[static_cast<int>(type)];
}

//
GLenum Utils::getShaderType(ShaderType type)
{
	static GLenum shader_types[] =
	{
		// Graphics pipeline
		GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER,

		// Compute pipeline
		GL_COMPUTE_SHADER,

		// Raytracing pipeline
		GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE,
	};

	if (type >= ShaderType::RAY_GENERATION && type <= ShaderType::CALLABLE)
		Log::warning("opengl::Utils::getShaderType(): raytracing shaders are not supported in OpenGL\n");

	return shader_types[static_cast<int>(type)];
}

GLenum Utils::getShaderStageBitmask(ShaderType type)
{
	static GLenum stage_bitmask[] =
	{
		// Graphics pipeline
		GL_VERTEX_SHADER_BIT, GL_TESS_CONTROL_SHADER_BIT, GL_TESS_EVALUATION_SHADER_BIT, GL_GEOMETRY_SHADER_BIT, GL_FRAGMENT_SHADER_BIT,

		// Compute pipeline
		GL_COMPUTE_SHADER_BIT,

		// Raytracing pipeline
		GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE,
	};

	if (type >= ShaderType::RAY_GENERATION && type <= ShaderType::CALLABLE)
		Log::warning("opengl::Utils::getShaderStageBitmask(): raytracing shaders are not supported in OpenGL\n");

	return stage_bitmask[static_cast<int>(type)];
}

//
GLint Utils::getTextureWrapMode(SamplerWrapMode mode)
{
	static GLenum wrap_modes[] =
	{
		GL_REPEAT,
		GL_CLAMP_TO_EDGE,
	};

	return wrap_modes[static_cast<int>(mode)];
}

GLenum Utils::getDefaultTextureMinFilter(GLenum internal_format)
{
	switch (internal_format)
	{
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32F:
		case GL_DEPTH24_STENCIL8:
		case GL_DEPTH32F_STENCIL8:
		case GL_RGB9_E5:
		case GL_R11F_G11F_B10F: return GL_NEAREST;
	}

	return GL_LINEAR;
}

GLenum Utils::getDefaultTextureMagFilter(GLenum internal_format)
{
	switch (internal_format)
	{
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32F:
		case GL_DEPTH24_STENCIL8:
		case GL_DEPTH32F_STENCIL8:
		case GL_RGB9_E5:
		case GL_R11F_G11F_B10F: return GL_NEAREST;
	}

	return GL_LINEAR;
}

GLenum Utils::getDefaultTextureWrapMode(GLenum type)
{
	switch (type)
	{
		case GL_TEXTURE_2D:
		case GL_TEXTURE_2D_ARRAY:
		case GL_TEXTURE_3D: return GL_REPEAT;
	}

	return GL_CLAMP_TO_EDGE;
}

void Utils::setDefaulTextureParameters(Texture *texture)
{
	GLenum min_filter = getDefaultTextureMinFilter(texture->internal_format);
	GLenum mag_filter = getDefaultTextureMagFilter(texture->internal_format);
	GLenum wrap_mode = getDefaultTextureWrapMode(texture->type);

	glBindTexture(texture->type, texture->id);
	glTexParameteri(texture->type, GL_TEXTURE_WRAP_S, wrap_mode);
	glTexParameteri(texture->type, GL_TEXTURE_WRAP_T, wrap_mode);
	glTexParameteri(texture->type, GL_TEXTURE_WRAP_R, wrap_mode);
	glTexParameteri(texture->type, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(texture->type, GL_TEXTURE_MAG_FILTER, mag_filter);
}

//
Buffer *Utils::createImmutableBuffer(GLenum type, GLsizei size, const GLvoid *data, GLbitfield usage_flags)
{
	Buffer *buffer = new Buffer();

	buffer->type = type;
	buffer->size = size;
	buffer->immutable = GL_TRUE;
	buffer->immutable_usage_flags = usage_flags;

	glGenBuffers(1, &buffer->id);
	glBindBuffer(type, buffer->id);
	glBufferStorage(type, size, data, usage_flags);
	glBindBuffer(type, 0);

	return buffer;
}

Buffer *Utils::createMutableBuffer(GLenum type, GLsizei size, const GLvoid *data, GLenum usage_hint)
{
	Buffer *buffer = new Buffer();

	buffer->type = type;
	buffer->size = size;
	buffer->immutable = GL_FALSE;
	buffer->mutable_usage_hint = usage_hint;

	glGenBuffers(1, &buffer->id);
	glBindBuffer(type, buffer->id);
	glBufferData(type, size, data, usage_hint);
	glBindBuffer(type, 0);

	return buffer;
}

void Utils::destroyBuffer(Buffer *buffer)
{
	assert(buffer);

	glDeleteBuffers(1, &buffer->id);
	buffer->id = 0;

	delete buffer;
}

}
