#pragma once

#include <render/backend/opengl/Driver.h>

namespace render::backend::opengl
{

class Utils
{
public:
	//
	static GLenum getInternalFormat(Format format);
	static GLenum getFormat(Format format);
	static GLenum getPixelType(Format format);
	static GLint getPixelSize(Format format);

	//
	static GLenum getFramebufferDepthStencilAttachmentType(GLenum internal_format);

	//
	static GLint getAttributeComponents(Format format);
	static GLenum getAttributeType(Format format);
	static GLboolean isAttributeNormalized(Format format);

	//
	static GLint getIndexFormat(IndexFormat format);
	static GLint getIndexSize(IndexFormat format);

	//
	static GLint getSampleCount(Multisample samples);
	static Multisample getMultisample(GLint sample_count);

	//
	static GLenum getDepthCompareFunc(DepthCompareFunc func);

	//
	static GLenum getBlendFactor(BlendFactor factor);

	//
	static GLenum getCullMode(CullMode mode);

	//
	static GLenum getPrimitiveType(RenderPrimitiveType type);
	static GLint getPrimitivePatchComponents(RenderPrimitiveType type);

	//
	static GLenum getShaderType(ShaderType type);
	static GLenum getShaderStageBitmask(ShaderType type);

	//
	static GLint getTextureWrapMode(SamplerWrapMode mode);
	static GLenum getDefaultTextureMinFilter(GLenum internal_format);
	static GLenum getDefaultTextureMagFilter(GLenum internal_format);
	static GLenum getDefaultTextureWrapMode(GLenum type);
	static void setDefaulTextureParameters(Texture *texture);

	//
	static Buffer *createImmutableBuffer(GLenum type, GLsizei size, const GLvoid *data, GLbitfield usage_flags = 0);
	static Buffer *createMutableBuffer(GLenum type, GLsizei size, const GLvoid *data, GLenum usage_hint = GL_STATIC_DRAW);
	static void destroyBuffer(Buffer *buffer);
};

}
