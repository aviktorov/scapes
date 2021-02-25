#include <common/Log.h>
#include <render/backend/opengl/Driver.h>
#include <render/backend/opengl/Utils.h>
#include <render/shaders/spirv/Compiler.h>

#include <glad_loader.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include <spirv_glsl.hpp>

namespace render::backend::opengl
{

// TODO: move to common utils
static void bitset_set(uint32_t &bitset, uint32_t bit, bool value)
{
	uint32_t mask = 1 << bit;
	bitset = (value) ? bitset | mask : bitset & ~mask;
}

static bool bitset_check(uint32_t bitset, uint32_t bit)
{
	uint32_t mask = 1 << bit;
	return (bitset & mask) != 0;
}

// TODO: move to command buffer utils
static void command_buffer_clear(CommandBuffer *gl_command_buffer)
{
	while (gl_command_buffer->first != nullptr)
	{
		Command *next = gl_command_buffer->first->next;

		delete gl_command_buffer->first;
		gl_command_buffer->first = next;
	}

	gl_command_buffer->first = nullptr;
	gl_command_buffer->last = nullptr;
}

static bool command_buffer_reset(CommandBuffer *gl_command_buffer)
{
	if (gl_command_buffer == nullptr)
		return false;

	command_buffer_clear(gl_command_buffer);
	gl_command_buffer->state = CommandBufferState::INITIAL;
	gl_command_buffer->push_constants_offset = 0;

	return true;
}

static bool command_buffer_begin(CommandBuffer *gl_command_buffer)
{
	if (gl_command_buffer == nullptr)
		return false;

	if (gl_command_buffer->state != CommandBufferState::INITIAL)
	{
		// TODO: log error
		return false;
	}

	gl_command_buffer->state = CommandBufferState::RECORDING;
	return true;
}

static bool command_buffer_end(CommandBuffer *gl_command_buffer)
{
	if (gl_command_buffer == nullptr)
		return false;

	if (gl_command_buffer->state != CommandBufferState::RECORDING)
	{
		// TODO: log error
		return false;
	}

	gl_command_buffer->state = CommandBufferState::EXECUTABLE;
	return true;
}

static Command *command_buffer_emit(CommandBuffer *gl_command_buffer, CommandType type)
{
	Command *command = new Command();
	memset(command, 0, sizeof(Command));

	command->next = nullptr;
	command->type = type;

	if (gl_command_buffer->first == nullptr)
		gl_command_buffer->first = command;
	else
		gl_command_buffer->last->next = command;

	gl_command_buffer->last = command;

	return command;
}

static void command_submit_set_viewport(const Command::Rect &data)
{
	glViewport(data.x, data.y, data.width, data.height);
}

static void command_submit_set_scissor(const Command::Rect &data)
{
	glScissor(data.x, data.y, data.width, data.height);
}

static void command_submit_set_depth_stencil_state(const Command::DepthStencilState &data)
{
	if (data.depth_test)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	glDepthMask(data.depth_write);
	glDepthFunc(data.depth_compare_func);
}

static void command_submit_set_blend_state(const Command::BlendState &data)
{
	if (data.enabled)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	glBlendFunc(data.src_factor, data.dst_factor);
}

static void command_submit_set_rasterizer_state(const Command::RasterizerState &data)
{
	if (data.cull_mode == GL_NONE)
	{
		glDisable(GL_CULL_FACE);
		return;
	}

	glEnable(GL_CULL_FACE);
	glCullFace(data.cull_mode);
}

static void command_submit_clear_color_buffer(const Command::ClearColorBuffer &data)
{
	glClearBufferfv(GL_COLOR, data.buffer, data.clear_color);
}

static void command_submit_clear_depth_stencil_buffer(const Command::ClearDepthStencilBuffer &data)
{
	glClearBufferfi(GL_DEPTH_STENCIL, 0, data.depth, data.stencil);
}

static void command_submit_blit_frame_buffer(const Command::BlitFrameBuffer &data)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, data.dst_fbo_id);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, data.src_fbo_id);
	glDrawBuffers(data.num_draw_buffers, data.draw_buffers);
	glBlitFramebuffer(0, 0, data.width, data.height, 0, 0, data.width, data.height, data.mask, GL_NEAREST);
}

static void command_submit_bind_frame_buffer(const Command::BindFrameBuffer &data)
{
	glBindFramebuffer(data.target, data.id);
}

static void command_submit_bind_uniform_buffer(const Command::BindUniformBuffer &data)
{
	glBindBufferRange(GL_UNIFORM_BUFFER, data.binding, data.id, data.offset, data.size);
}

static void command_submit_bind_texture(const Command::BindTexture &data)
{
	glActiveTexture(GL_TEXTURE0 + data.binding);
	glBindTexture(data.type, data.id);

}

static void command_submit_bind_shader(const Command::BindShader &data)
{
	glBindProgramPipeline(data.pipeline_id);
	glUseProgramStages(data.pipeline_id, data.shader_stages, data.shader_id);
}

static void command_submit_bind_vertex_buffer(const Command::BindVertexBuffer &data)
{
	glBindVertexArray(data.vao_id);
}

static void command_submit_bind_index_buffer(const Command::BindIndexBuffer &data)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.id);
}

static void command_submit_draw_indexed_primitive(const Command::DrawIndexedPrimitive &data)
{
	glDrawElementsInstancedBaseVertex(
		data.primitive_type,
		data.num_indices,
		data.index_format,
		static_cast<const char *>(0) + data.base_index,
		data.num_instances,
		data.base_vertex
	);
}

static bool command_buffer_submit(CommandBuffer *gl_command_buffer)
{
	if (gl_command_buffer == nullptr)
		return false;

	if (gl_command_buffer->state != CommandBufferState::EXECUTABLE)
	{
		// TODO: log error
		return false;
	}

	const Command *command = gl_command_buffer->first;

	while (command)
	{
		switch (command->type)
		{
			case CommandType::SET_VIEWPORT: command_submit_set_viewport(command->viewport); break;
			case CommandType::SET_SCISSOR: command_submit_set_scissor(command->scissor); break;
			case CommandType::SET_DEPTH_STENCIL_STATE: command_submit_set_depth_stencil_state(command->depth_stencil_state); break;
			case CommandType::SET_BLEND_STATE: command_submit_set_blend_state(command->blend_state); break;
			case CommandType::SET_RASTERIZER_STATE: command_submit_set_rasterizer_state(command->rasterizer_state); break;
			case CommandType::CLEAR_COLOR_BUFFER: command_submit_clear_color_buffer(command->clear_color_buffer); break;
			case CommandType::CLEAR_DEPTHSTENCIL_BUFFER: command_submit_clear_depth_stencil_buffer(command->clear_depth_stencil_buffer); break;
			case CommandType::BLIT_FRAME_BUFFER: command_submit_blit_frame_buffer(command->blit_frame_buffer); break;
			case CommandType::BIND_FRAME_BUFFER: command_submit_bind_frame_buffer(command->bind_frame_buffer); break;
			case CommandType::BIND_UNIFORM_BUFFER: command_submit_bind_uniform_buffer(command->bind_uniform_buffer); break;
			case CommandType::BIND_TEXTURE: command_submit_bind_texture(command->bind_texture); break;
			case CommandType::BIND_SHADER: command_submit_bind_shader(command->bind_shader); break;
			case CommandType::BIND_INDEX_BUFFER: command_submit_bind_index_buffer(command->bind_index_buffer); break;
			case CommandType::BIND_VERTEX_BUFFER: command_submit_bind_vertex_buffer(command->bind_vertex_buffer); break;
			case CommandType::DRAW_INDEXED_PRIMITIVE: command_submit_draw_indexed_primitive(command->draw_indexed_primitive); break;
			default: assert(false && "Unsupported command"); return false;
		}

		command = command->next;
	}

	gl_command_buffer->state = CommandBufferState::INVALID;
	return true;
}

//
static void GLAPIENTRY debugLog(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	const void *user_data
)
{
	Log::error("---------------\n");
	Log::error("opengl::Driver: (%d) %s\n", id, message);

	switch (source)
	{
		case GL_DEBUG_SOURCE_API:				Log::error("Source: API\n"); break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		Log::error("Source: Window System\n"); break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:	Log::error("Source: Shader Compiler\n"); break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:		Log::error("Source: Third Party\n"); break;
		case GL_DEBUG_SOURCE_APPLICATION:		Log::error("Source: Application\n"); break;
		case GL_DEBUG_SOURCE_OTHER:				Log::error("Source: Other\n"); break;
	}

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:				Log::error("Type: Error\n"); break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:	Log::error("Type: Deprecated Behaviour\n"); break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:	Log::error("Type: Undefined Behaviour\n"); break;
		case GL_DEBUG_TYPE_PORTABILITY:			Log::error("Type: Portability\n"); break;
		case GL_DEBUG_TYPE_PERFORMANCE:			Log::error("Type: Performance\n"); break;
		case GL_DEBUG_TYPE_MARKER:				Log::error("Type: Marker\n"); break;
		case GL_DEBUG_TYPE_PUSH_GROUP:			Log::error("Type: Push Group\n"); break;
		case GL_DEBUG_TYPE_POP_GROUP:			Log::error("Type: Pop Group\n"); break;
		case GL_DEBUG_TYPE_OTHER:				Log::error("Type: Other\n"); break;
	}

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:			Log::error("Severity: high\n"); break;
		case GL_DEBUG_SEVERITY_MEDIUM:			Log::error("Severity: medium\n"); break;
		case GL_DEBUG_SEVERITY_LOW:				Log::error("Severity: low\n"); break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:	Log::error("Severity: notification\n"); break;
	}
 	Log::error("\n");
}

//
Driver::Driver(const char *application_name, const char *engine_name)
{
	init();
}

Driver::~Driver()
{
	shutdown();
}

//
bool Driver::init()
{
	if (!gladLoaderInit())
	{
		Log::fatal("opengl::Driver::init(): failed to initialize GLAD loader\n");
		return false;
	}

	// TODO: debug context
	bool debug = false;

	if (!Platform::init(debug))
	{
		Log::fatal("opengl::Driver::init(): failed to initialize platform\n");
		return false;
	}

	if (!gladLoadGLLoader(&gladLoaderProc))
	{
		Log::fatal("opengl::Driver::init(): failed to load OpenGL\n");
		return false;
	}

	if (!GLAD_GL_VERSION_4_5)
	{
		Log::fatal("opengl::Driver::init(): this device does not support OpenGL 4.5\n");
		return false;
	}

	glEnable(GL_MULTISAMPLE);
	glEnable(GL_SCISSOR_TEST);

	if (debug)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(debugLog, nullptr);
	}

	memset(&pipeline_state, 0, sizeof(PipelineState));
	memset(&pipeline_state_overrides, 0, sizeof(PipelineStateOverrides));
	memset(&render_pass_state, 0, sizeof(RenderPassState));
	memset(&push_constants, 0, MAX_PUSH_CONSTANT_SIZE);
	push_constants_size = 0;

	pipeline_state.depth_compare_func = GL_LEQUAL;
	pipeline_state.blend_src_factor = GL_ZERO;
	pipeline_state.blend_dst_factor = GL_ZERO;

	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniform_buffer_offset_alignment);

	return true;
}

void Driver::shutdown()
{
	glDeleteProgramPipelines(1, &graphics_pipeline_id);
	graphics_pipeline_id = GL_NONE;

	Platform::shutdown();
}

void Driver::fetchGraphicsPipeline()
{
	if (graphics_pipeline_id != GL_NONE)
		return;

	glGenProgramPipelines(1, &graphics_pipeline_id);
	glBindProgramPipeline(graphics_pipeline_id);

	assert(glIsProgramPipeline(graphics_pipeline_id) == GL_TRUE);
}

void Driver::flushPipelineState(CommandBuffer *gl_command_buffer)
{
	if (!pipeline_dirty)
		return;

	pipeline_dirty = false;

	// viewport
	if (pipeline_state_overrides.viewport)
	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::SET_VIEWPORT);
		command->viewport.x = pipeline_state.viewport_x;
		command->viewport.y = pipeline_state.viewport_y;
		command->viewport.width = pipeline_state.viewport_width;
		command->viewport.height = pipeline_state.viewport_height;
	}

	// scissor
	if (pipeline_state_overrides.scissor)
	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::SET_SCISSOR);
		command->scissor.x = pipeline_state.scissor_x;
		command->scissor.y = pipeline_state.scissor_y;
		command->scissor.width = pipeline_state.scissor_width;
		command->scissor.height = pipeline_state.scissor_height;
	}

	// depth state
	if (pipeline_state_overrides.depth_stencil_state)
	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::SET_DEPTH_STENCIL_STATE);
		command->depth_stencil_state.depth_test = pipeline_state.depth_test;
		command->depth_stencil_state.depth_write = pipeline_state.depth_write;
		command->depth_stencil_state.depth_compare_func = pipeline_state.depth_compare_func;
	}

	// blending
	if (pipeline_state_overrides.blend_state)
	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::SET_BLEND_STATE);
		command->blend_state.enabled = pipeline_state.blend;
		command->blend_state.src_factor = pipeline_state.blend_src_factor;
		command->blend_state.dst_factor = pipeline_state.blend_dst_factor;
	}

	// rasterizer
	if (pipeline_state_overrides.rasterizer_state)
	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::SET_RASTERIZER_STATE);
		command->rasterizer_state.cull_mode = pipeline_state.cull_mode;
	}

	// bind sets
	for (uint16_t i = 0; i < pipeline_state.num_bind_sets; ++i)
	{
		if (!bitset_check(pipeline_state_overrides.bound_bind_sets, i))
			continue;

		const BindSet *gl_bind_set = pipeline_state.bound_bind_sets[i];

		GLuint base_binding = i * BindSet::MAX_BINDINGS;

		for (uint32_t j = 0; j < BindSet::MAX_BINDINGS; ++j)
		{
			if (!bitset_check(gl_bind_set->binding_used, j))
				continue;

			BindSet::DataType type = gl_bind_set->types[j];
			const BindSet::Data &data = gl_bind_set->datas[j];

			switch (type)
			{
				case BindSet::DataType::TEXTURE:
				{
					Command *command = command_buffer_emit(gl_command_buffer, CommandType::BIND_TEXTURE);

					// TODO: get texture view from cache

					command->bind_texture.binding = base_binding + j;
					command->bind_texture.id = data.texture.id;
					command->bind_texture.type = data.texture.type;
				}
				break;
				case BindSet::DataType::UNIFORM_BUFFER:
				{
					Command *command = command_buffer_emit(gl_command_buffer, CommandType::BIND_UNIFORM_BUFFER);
					command->bind_uniform_buffer.binding = base_binding + j + UNIFORM_BUFFER_BINDING_OFFSET;
					command->bind_uniform_buffer.id = data.ubo.id;
					command->bind_uniform_buffer.offset = data.ubo.offset;
					command->bind_uniform_buffer.size = data.ubo.size;
				}
				break;
			}
		}
	}

	// shaders
	fetchGraphicsPipeline();

	for (uint16_t i = 0; i < MAX_SHADERS; ++i)
	{
		if (!bitset_check(pipeline_state_overrides.bound_shaders, i))
			continue;

		Command *command = command_buffer_emit(gl_command_buffer, CommandType::BIND_SHADER);
		command->bind_shader.pipeline_id = graphics_pipeline_id;
		command->bind_shader.shader_id = pipeline_state.bound_shaders[i];
		command->bind_shader.shader_stages = Utils::getShaderStageBitmask(static_cast<ShaderType>(i));
	}

	pipeline_state_overrides = {};
}

//
backend::VertexBuffer *Driver::createVertexBuffer(
	BufferType type,
	uint16_t vertex_size,
	uint32_t num_vertices,
	uint8_t num_attributes,
	const backend::VertexAttribute *attributes,
	const void *data
)
{
	VertexBuffer *result = new VertexBuffer();

	GLbitfield usage_flags = 0;

	if (type == BufferType::DYNAMIC)
		usage_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

	result->data = Utils::createImmutableBuffer(GL_ARRAY_BUFFER, vertex_size * num_vertices, data, usage_flags);
	result->num_vertices = num_vertices;
	result->vertex_size = vertex_size;

	glGenVertexArrays(1, &result->vao_id);
	glBindVertexArray(result->vao_id);

	glBindBuffer(result->data->type, result->data->id);
	for(int i = 0; i < num_attributes; i++)
	{
		const VertexAttribute &attribute = attributes[i];

		GLint num_elements = Utils::getAttributeComponents(attribute.format);
		GLenum type = Utils::getAttributeType(attribute.format);
		GLboolean normalized = Utils::isAttributeNormalized(attribute.format);

		glVertexAttribPointer(i, num_elements, type, normalized, vertex_size, (const char *)0 + attribute.offset);
		glEnableVertexAttribArray(i);
	}

	glBindBuffer(result->data->type, result->data->id);
	glBindVertexArray(0);

	return result;
}

backend::IndexBuffer *Driver::createIndexBuffer(
	BufferType type,
	IndexFormat index_format,
	uint32_t num_indices,
	const void *data
)
{
	IndexBuffer *result = new IndexBuffer();

	GLbitfield usage_flags = 0;

	if (type == BufferType::DYNAMIC)
		usage_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

	result->data = Utils::createImmutableBuffer(GL_ELEMENT_ARRAY_BUFFER, Utils::getIndexSize(index_format) * num_indices, data, usage_flags);
	result->num_indices = num_indices;
	result->index_format = Utils::getIndexFormat(index_format);

	return result;
}

//
backend::Texture *Driver::createTexture2D(
	uint32_t width,
	uint32_t height,
	uint32_t num_mipmaps,
	Format format,
	const void *data,
	uint32_t num_data_mipmaps
)
{
	Texture *result = new Texture();

	result->type = GL_TEXTURE_2D;
	result->internal_format = Utils::getInternalFormat(format);
	result->width = width;
	result->height = height;
	result->mips = static_cast<GLint>(num_mipmaps);

	glGenTextures(1, &result->id);
	glBindTexture(result->type, result->id);
	glTexStorage2D(result->type, result->mips, result->internal_format, result->width, result->height);

	if (data != nullptr)
	{
		GLint pixel_size = Utils::getPixelSize(format);
		GLenum pixel_type = Utils::getPixelType(format);
		GLenum gl_format = Utils::getFormat(format);
		const unsigned char *mip_data = reinterpret_cast<const unsigned char *>(data);

		GLint mip_width = result->width;
		GLint mip_height = result->height;

		for (uint32_t mip = 0; mip < num_data_mipmaps; mip++)
		{
			glTexSubImage2D(result->type, mip, 0, 0, mip_width, mip_height, gl_format, pixel_type, mip_data);

			mip_data += mip_width * mip_height * pixel_size;
			mip_width = std::max<GLint>(1, mip_width / 2);
			mip_height = std::max<GLint>(1, mip_height / 2);
		}
	}

	Utils::setDefaulTextureParameters(result);
	return result;
}

backend::Texture *Driver::createTexture2DMultisample(
	uint32_t width,
	uint32_t height,
	Format format,
	Multisample samples
)
{
	Texture *result = new Texture();

	result->type = GL_TEXTURE_2D_MULTISAMPLE;
	result->internal_format = Utils::getInternalFormat(format);
	result->width = width;
	result->height = height;
	result->num_samples = Utils::getSampleCount(samples);

	glGenTextures(1, &result->id);
	glBindTexture(result->type, result->id);

	glTexImage2DMultisample(result->type, result->num_samples, result->internal_format, result->width, result->height, GL_FALSE);

	Utils::setDefaulTextureParameters(result);
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
	assert(num_layers > 0);

	Texture *result = new Texture();

	result->type = GL_TEXTURE_2D_ARRAY;
	result->internal_format = Utils::getInternalFormat(format);
	result->width = width;
	result->height = height;
	result->layers = num_layers;
	result->mips = num_mipmaps;

	glGenTextures(1, &result->id);
	glBindTexture(result->type, result->id);
	glTexStorage3D(result->type, result->mips, result->internal_format, result->width, result->height, result->layers);

	if (data != nullptr)
	{
		GLint pixel_size = Utils::getPixelSize(format);
		GLenum pixel_type = Utils::getPixelType(format);
		GLenum gl_format = Utils::getFormat(format);
		const unsigned char *mip_data = reinterpret_cast<const unsigned char *>(data);

		for (uint32_t layer = 0; layer < num_data_layers; layer++)
		{
			GLint mip_width = result->width;
			GLint mip_height = result->height;
			for (uint32_t mip = 0; mip < num_data_mipmaps; mip++)
			{
				glTexSubImage3D(result->type, mip, 0, 0, layer, mip_width, mip_height, 1, gl_format, pixel_type, mip_data);

				mip_data += mip_width * mip_height * pixel_size;
				mip_width = std::max<GLint>(1, mip_width / 2);
				mip_height = std::max<GLint>(1, mip_height / 2);
			}
		}
	}

	Utils::setDefaulTextureParameters(result);
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
	Texture *result = new Texture();

	result->type = GL_TEXTURE_3D;
	result->internal_format = Utils::getInternalFormat(format);
	result->width = width;
	result->height = height;
	result->depth = depth;
	result->mips = num_mipmaps;

	glGenTextures(1, &result->id);
	glBindTexture(result->type, result->id);
	glTexStorage3D(result->type, result->mips, result->internal_format, result->width, result->height, result->depth);

	if (data != nullptr)
	{
		GLint pixel_size = Utils::getPixelSize(format);
		GLenum pixel_type = Utils::getPixelType(format);
		GLenum gl_format = Utils::getFormat(format);
		const unsigned char *mip_data = reinterpret_cast<const unsigned char *>(data);

		GLint mip_width = result->width;
		GLint mip_height = result->height;
		GLint mip_depth = result->depth;

		for (uint32_t mip = 0; mip < num_data_mipmaps; mip++)
		{
			glTexSubImage3D(result->type, mip, 0, 0, 0, mip_width, mip_height, mip_depth, gl_format, pixel_type, mip_data);

			mip_data += mip_width * mip_height * mip_depth * pixel_size;
			mip_width = std::max<GLint>(1, mip_width / 2);
			mip_height = std::max<GLint>(1, mip_height / 2);
			mip_depth =  std::max<GLint>(1, mip_depth / 2);
		}
	}

	Utils::setDefaulTextureParameters(result);
	return result;
}

backend::Texture *Driver::createTextureCube(
	uint32_t size,
	uint32_t num_mipmaps,
	Format format,
	const void *data,
	uint32_t num_data_mipmaps
)
{
	Texture *result = new Texture();

	result->type = GL_TEXTURE_CUBE_MAP;
	result->internal_format = Utils::getInternalFormat(format);
	result->width = size;
	result->height = size;
	result->layers = 6;
	result->mips = num_mipmaps;

	glGenTextures(1, &result->id);
	glBindTexture(result->type, result->id);

	GLenum pixel_type = Utils::getPixelType(format);
	GLenum gl_format = Utils::getFormat(format);

	GLint mip_width = result->width;
	GLint mip_height = result->height;

	for (uint32_t mip = 0; mip < num_mipmaps; mip++)
	{
		for (uint32_t layer = 0; layer < 6; layer++)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, mip, result->internal_format, mip_width, mip_height, 0, gl_format, pixel_type, nullptr);

		mip_width = std::max<GLint>(1, mip_width / 2);
		mip_height = std::max<GLint>(1, mip_height / 2);
	}

	if (data != nullptr)
	{
		GLint pixel_size = Utils::getPixelSize(format);
		const unsigned char *mip_data = reinterpret_cast<const unsigned char *>(data);

		for (uint32_t layer = 0; layer < 6; layer++)
		{
			mip_width = result->width;
			mip_height = result->height;

			for (uint32_t mip = 0; mip < num_data_mipmaps; mip++)
			{
				glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, mip, 0, 0, mip_width, mip_height, gl_format, pixel_type, mip_data);

				mip_data += mip_width * mip_height * pixel_size;
				mip_width = std::max<GLint>(1, mip_width / 2);
				mip_height = std::max<GLint>(1, mip_height / 2);
			}
		}
	}

	glTexParameteri(result->type, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(result->type, GL_TEXTURE_MAX_LEVEL, result->mips - 1);

	Utils::setDefaulTextureParameters(result);
	return result;
}

//
backend::FrameBuffer *Driver::createFrameBuffer(
	uint8_t num_color_attachments,
	const FrameBufferAttachment *color_attachments,
	const FrameBufferAttachment *depth_stencil_attachment
)
{
	assert((depth_stencil_attachment != nullptr) || (num_color_attachments > 0 && color_attachments != nullptr));

	// Fill struct fields
	FrameBuffer *result = new FrameBuffer();

	result->num_color_attachments = 0;
	result->num_resolve_color_attachments = 0;
	result->main_fbo_id = 0;
	result->resolve_fbo_id = 0;
	result->width = 0;
	result->height = 0;

	for (uint8_t i = 0; i < num_color_attachments; ++i)
	{
		const FrameBufferAttachment &attachment = color_attachments[i];
		const Texture *gl_texture = static_cast<const Texture *>(attachment.texture);

		uint8_t target_index = 0;
		FrameBufferColorAttachment *target_attachments = nullptr;
		uint32_t *target_indices = nullptr;

		if (attachment.resolve_attachment)
		{
			target_index = result->num_resolve_color_attachments;
			target_attachments = result->resolve_color_attachments;
			target_indices = result->resolve_color_attachment_indices;
		}
		else
		{
			target_index = result->num_color_attachments;
			target_attachments = result->color_attachments;
			target_indices = result->color_attachment_indices;
		}

		FrameBufferColorAttachment &target_attachment = target_attachments[target_index];
		target_indices[target_index] = i;

		target_attachment.id = gl_texture->id;
		target_attachment.texture_type = gl_texture->type;
		target_attachment.type = GL_COLOR_ATTACHMENT0 + target_index;
		target_attachment.base_mip = attachment.base_mip;
		target_attachment.base_layer = attachment.base_layer;
		target_attachment.num_layers = attachment.num_layers;

		result->width = std::max<uint32_t>(result->width, gl_texture->width);
		result->height = std::max<uint32_t>(result->height, gl_texture->height);

		if (attachment.resolve_attachment)
			result->num_resolve_color_attachments++;
		else
			result->num_color_attachments++;
	}

	if (depth_stencil_attachment != nullptr)
	{
		const Texture *gl_texture = static_cast<const Texture *>(depth_stencil_attachment->texture);

		result->depth_stencil_attachment.id = gl_texture->id;
		result->depth_stencil_attachment.type = Utils::getFramebufferDepthStencilAttachmentType(gl_texture->internal_format);
		result->depth_stencil_attachment_index = num_color_attachments;
	}

	auto create_fbo = [](
		GLuint *fbo_id,
		uint8_t num_color_attachments,
		FrameBufferColorAttachment *color_attachments,
		FrameBufferDepthStencilAttachment *depth_stencil_attachment
	)
	{
		assert(fbo_id);

		glGenFramebuffers(1, fbo_id);
		glBindFramebuffer(GL_FRAMEBUFFER, *fbo_id);

		GLenum draw_buffers[FrameBuffer::MAX_COLOR_ATTACHMENTS];

		for (uint8_t i = 0; i < num_color_attachments; ++i)
		{
			const FrameBufferColorAttachment &attachment = color_attachments[i];

			switch (attachment.texture_type)
			{
				case GL_TEXTURE_2D:
				case GL_TEXTURE_2D_MULTISAMPLE:
					glFramebufferTexture2D(GL_FRAMEBUFFER, attachment.type, attachment.texture_type, attachment.id, attachment.base_mip);
				break;
				case GL_TEXTURE_CUBE_MAP:
				{
					if (attachment.num_layers == 6)
						glFramebufferTexture(GL_FRAMEBUFFER, attachment.type, attachment.id, attachment.base_mip);
					else
						glFramebufferTexture2D(GL_FRAMEBUFFER, attachment.type, GL_TEXTURE_CUBE_MAP_POSITIVE_X + attachment.base_layer, attachment.id, attachment.base_mip);
				}
				break;
				case GL_TEXTURE_2D_ARRAY:
				case GL_TEXTURE_3D:
					glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment.type, attachment.id, attachment.base_mip, attachment.base_layer);
				break;
			}

			draw_buffers[i] = attachment.type;
		}

		if (depth_stencil_attachment && depth_stencil_attachment->id != 0)
			glFramebufferTexture2D(GL_FRAMEBUFFER, depth_stencil_attachment->type, GL_TEXTURE_2D, depth_stencil_attachment->id, 0);

		glDrawBuffers(num_color_attachments, draw_buffers);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		return status;
	};

	// Create main FBO
	GLenum status = create_fbo(&result->main_fbo_id, result->num_color_attachments, result->color_attachments, &result->depth_stencil_attachment);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		Log::error("opengl::Driver::createFramebuffer(): main framebuffer is not complete, error: %d\n", status);
		destroyFrameBuffer(result);
		return nullptr;
	}

	// Create resolve FBO
	if (result->num_resolve_color_attachments > 0)
	{
		status = create_fbo(&result->resolve_fbo_id, result->num_resolve_color_attachments, result->resolve_color_attachments, nullptr);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			Log::error("opengl::Driver::createFramebuffer(): resolve framebuffer is not complete, error: %d\n", status);
			destroyFrameBuffer(result);
			return nullptr;
		}
	}

	return result;
}

backend::CommandBuffer *Driver::createCommandBuffer(
	CommandBufferType type
)
{
	CommandBuffer *result = new CommandBuffer();
	result->type = type;

	GLbitfield usage_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

	result->push_constants = Utils::createImmutableBuffer(GL_UNIFORM_BUFFER, CommandBuffer::MAX_PUSH_CONSTANT_BUFFER_SIZE, nullptr, usage_flags);

	glBindBuffer(GL_UNIFORM_BUFFER, result->push_constants->id);
	result->push_constants_mapped_memory = glMapBufferRange(GL_UNIFORM_BUFFER, 0, CommandBuffer::MAX_PUSH_CONSTANT_BUFFER_SIZE, usage_flags);
	result->push_constants_offset = 0;

	// TODO: sync primitives

	return result;
}

//
backend::UniformBuffer *Driver::createUniformBuffer(
	BufferType type,
	uint32_t size,
	const void *data
)
{
	UniformBuffer *result = new UniformBuffer();

	GLbitfield usage_flags = 0;

	if (type == BufferType::DYNAMIC)
		usage_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

	result->data = Utils::createImmutableBuffer(GL_UNIFORM_BUFFER, size, data, usage_flags);

	return result;
}

//
backend::Shader *Driver::createShaderFromSource(
	ShaderType type,
	uint32_t size,
	const char *data,
	const char *path
)
{
	GLenum shader_type = Utils::getShaderType(type);
	GLuint shader = glCreateShader(shader_type);
	if (shader == 0)
	{
		Log::error("opengl::Driver::createShaderFromSource(): glCreateShader failed, path: \"%s\"\n", path);
		return nullptr;
	}

	GLint shader_size = static_cast<GLint>(size);
	const GLchar *shader_data = reinterpret_cast<const GLchar *>(data);

	glShaderSource(shader, 1, &shader_data, &shader_size);
	glCompileShader(shader);

	GLint compiled = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled)
	{
		GLint length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

		std::vector<GLchar> info_log(length);
		glGetShaderInfoLog(shader, length, &length, info_log.data());

		Log::error("opengl::Driver::createShaderFromSource(): compilation failed, path: \"%s\", error: %s\n", path, info_log.data());
	}

	GLuint program = glCreateProgram();
	if (program == 0)
	{
		glDeleteShader(shader);
		Log::error("opengl::Driver::createShaderFromSource(): glCreateProgram failed, path: \"%s\"\n", path);
		return nullptr;
	}
	glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);

	if (compiled)
	{
		glAttachShader(program, shader);
		glLinkProgram(program);
		glDetachShader(program, shader);
	}

	glDeleteShader(shader);

	GLint linked = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		GLint length = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

		std::vector<GLchar> info_log(length);
		glGetProgramInfoLog(program, length, &length, info_log.data());
		
		Log::error("opengl::Driver::createShaderFromSource(): linkage failed, path: \"%s\", error: %s\n", path, info_log.data());
	}

	Shader *result = new Shader();

	result->type = shader_type;
	result->id = program;
	result->compiled = compiled && linked;

	return result;
}

backend::Shader *Driver::createShaderFromIL(
	const shaders::ShaderIL *shader_il
)
{
	const shaders::spirv::ShaderIL *spirv_shader_il = static_cast<const shaders::spirv::ShaderIL *>(shader_il);

	spirv_cross::CompilerGLSL glsl(spirv_shader_il->bytecode_data, spirv_shader_il->bytecode_size / sizeof(uint32_t));

	spirv_cross::ShaderResources resources = glsl.get_shader_resources();

	for (auto &resource : resources.sampled_images)
	{
		uint32_t set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
		uint32_t binding = glsl.get_decoration(resource.id, spv::DecorationBinding);

		glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);
		glsl.set_decoration(resource.id, spv::DecorationBinding, set * BindSet::MAX_BINDINGS + binding);
	}

	for (auto &resource : resources.uniform_buffers)
	{
		uint32_t set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
		uint32_t binding = glsl.get_decoration(resource.id, spv::DecorationBinding);

		glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);
		glsl.set_decoration(resource.id, spv::DecorationBinding, set * BindSet::MAX_BINDINGS + binding + UNIFORM_BUFFER_BINDING_OFFSET);
	}

	assert(resources.push_constant_buffers.size() <= 1);
	for (auto &resource : resources.push_constant_buffers)
	{
		glsl.set_name(resource.id, "scapesPushConstants");

		glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);
		glsl.set_decoration(resource.id, spv::DecorationBinding, 0);
	}

	spirv_cross::CompilerGLSL::Options options;
	options.version = 450;
	options.es = false;
	options.separate_shader_objects = true;
	options.emit_push_constant_as_uniform_buffer = true;
	glsl.set_common_options(options);

	std::string source = glsl.compile();

	return createShaderFromSource(spirv_shader_il->type, static_cast<uint32_t>(source.size()), source.c_str());
}

//
backend::BindSet *Driver::createBindSet(
)
{
	BindSet *result = new BindSet();
	memset(result, 0, sizeof(BindSet));

	return result;
}

//
backend::SwapChain *Driver::createSwapChain(
	void *native_window,
	uint32_t width,
	uint32_t height,
	Multisample samples
)
{
	assert(native_window);
	GLint gl_samples = Utils::getSampleCount(samples);

	SwapChain *result = new SwapChain();
	result->surface = Platform::createSurface(native_window);
	result->num_images = 2;
	result->width = width;
	result->height = height;

	if (gl_samples > 1)
	{
		GLenum color_internal_format = Utils::getInternalFormat(result->color_format);
		GLenum depth_stencil_internal_format = Utils::getInternalFormat(result->depth_stencil_format);

		glGenRenderbuffers(1, &result->msaa_color_id);
		glGenRenderbuffers(1, &result->msaa_depth_stencil_id);

		glBindRenderbuffer(GL_RENDERBUFFER, result->msaa_color_id);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, gl_samples, color_internal_format, width, height);

		glBindRenderbuffer(GL_RENDERBUFFER, result->msaa_depth_stencil_id);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, gl_samples, depth_stencil_internal_format, width, height);

		glGenFramebuffers(1, &result->msaa_fbo_id);
		glBindFramebuffer(GL_FRAMEBUFFER, result->msaa_fbo_id);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, result->msaa_color_id);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, result->msaa_depth_stencil_id);

		GLenum draw_buffer = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &draw_buffer);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			Log::error("opengl::Driver::createSwapChain(): msaa framebuffer is not complete, error: %d\n", status);
			destroySwapChain(result);
			return nullptr;
		}
	}

	return result;
}

//
void Driver::destroyVertexBuffer(backend::VertexBuffer *vertex_buffer)
{
	if (vertex_buffer == nullptr)
		return;

	VertexBuffer *gl_buffer = static_cast<VertexBuffer *>(vertex_buffer);
	Utils::destroyBuffer(gl_buffer->data);

	glDeleteVertexArrays(1, &gl_buffer->vao_id);
	gl_buffer->vao_id = 0;

	delete gl_buffer;
}

void Driver::destroyIndexBuffer(backend::IndexBuffer *index_buffer)
{
	if (index_buffer == nullptr)
		return;

	IndexBuffer *gl_buffer = static_cast<IndexBuffer *>(index_buffer);
	Utils::destroyBuffer(gl_buffer->data);

	delete gl_buffer;
}

void Driver::destroyTexture(backend::Texture *texture)
{
	if (texture == nullptr)
		return;

	Texture *gl_texture = static_cast<Texture *>(texture);

	glDeleteTextures(1, &gl_texture->id);
	gl_texture->id = 0;

	delete gl_texture;
}

void Driver::destroyFrameBuffer(backend::FrameBuffer *frame_buffer)
{
	if (frame_buffer == nullptr)
		return;

	FrameBuffer *gl_buffer = static_cast<FrameBuffer *>(frame_buffer);

	GLuint ids[] = { gl_buffer->main_fbo_id, gl_buffer->resolve_fbo_id };
	glDeleteFramebuffers(2, ids);

	gl_buffer->main_fbo_id = 0;
	gl_buffer->resolve_fbo_id = 0;

	delete gl_buffer;
}

void Driver::destroyCommandBuffer(backend::CommandBuffer *command_buffer)
{
	if (command_buffer == nullptr)
		return;

	CommandBuffer *gl_command_buffer = static_cast<CommandBuffer *>(command_buffer);
	command_buffer_clear(gl_command_buffer);

	glBindBuffer(GL_UNIFORM_BUFFER, gl_command_buffer->push_constants->id);
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	Utils::destroyBuffer(gl_command_buffer->push_constants);

	gl_command_buffer->first = nullptr;
	gl_command_buffer->last = nullptr;
	gl_command_buffer->push_constants = nullptr;
	gl_command_buffer->push_constants_mapped_memory = nullptr;

	delete command_buffer;
	command_buffer = nullptr;
}

void Driver::destroyUniformBuffer(backend::UniformBuffer *uniform_buffer)
{
	if (uniform_buffer == nullptr)
		return;

	UniformBuffer *gl_buffer = static_cast<UniformBuffer *>(uniform_buffer);
	Utils::destroyBuffer(gl_buffer->data);

	delete gl_buffer;
}

void Driver::destroyShader(backend::Shader *shader)
{
	if (shader == nullptr)
		return;

	Shader *gl_shader = static_cast<Shader *>(shader);

	glDeleteProgram(gl_shader->id);
	gl_shader->id = 0;

	delete gl_shader;
}

void Driver::destroyBindSet(backend::BindSet *bind_set)
{
	if (bind_set == nullptr)
		return;

	BindSet *gl_bind_set = static_cast<BindSet *>(bind_set);
	memset(gl_bind_set, 0, sizeof(BindSet));

	delete bind_set;
	bind_set = nullptr;
}

void Driver::destroySwapChain(backend::SwapChain *swap_chain)
{
	if (swap_chain == nullptr)
		return;

	SwapChain *gl_swap_chain = static_cast<SwapChain *>(swap_chain);

	glDeleteFramebuffers(1, &gl_swap_chain->msaa_fbo_id);

	GLuint buffers[] = { gl_swap_chain->msaa_color_id, gl_swap_chain->msaa_depth_stencil_id };
	glDeleteRenderbuffers(2, buffers);

	gl_swap_chain->msaa_fbo_id = 0;
	gl_swap_chain->msaa_color_id = 0;
	gl_swap_chain->msaa_depth_stencil_id = 0;

	Platform::destroySurface(gl_swap_chain->surface);
	gl_swap_chain->surface = nullptr;

	delete gl_swap_chain;
}

//
Multisample Driver::getMaxSampleCount()
{
	GLint max_samples = 0;
	glGetIntegerv(GL_MAX_SAMPLES, &max_samples);

	return Utils::getMultisample(max_samples);
}

uint32_t Driver::getNumSwapChainImages(const backend::SwapChain *swap_chain)
{
	assert(swap_chain);
	const SwapChain *gl_swap_chain = static_cast<const SwapChain *>(swap_chain);

	return gl_swap_chain->num_images;
}

void Driver::setTextureSamplerWrapMode(backend::Texture *texture, SamplerWrapMode mode)
{
	assert(texture);
	Texture *gl_texture = static_cast<Texture *>(texture);

	GLint wrap_mode = Utils::getTextureWrapMode(mode);
	glTexParameteri(gl_texture->type, GL_TEXTURE_WRAP_S, wrap_mode);
	glTexParameteri(gl_texture->type, GL_TEXTURE_WRAP_T, wrap_mode);
	glTexParameteri(gl_texture->type, GL_TEXTURE_WRAP_R, wrap_mode);
}

void Driver::setTextureSamplerDepthCompare(backend::Texture *texture, bool enabled, DepthCompareFunc func)
{
	assert(texture);
	Texture *gl_texture = static_cast<Texture *>(texture);

	glTexParameteri(gl_texture->type, GL_TEXTURE_COMPARE_MODE, (enabled) ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE);
	glTexParameteri(gl_texture->type, GL_TEXTURE_COMPARE_FUNC, Utils::getDepthCompareFunc(func));
}

void Driver::generateTexture2DMipmaps(backend::Texture *texture)
{
	assert(texture);
	Texture *gl_texture = static_cast<Texture *>(texture);

	glBindTexture(gl_texture->type, gl_texture->id);
	glGenerateMipmap(gl_texture->type);
}

//
void *Driver::map(backend::VertexBuffer *buffer)
{
	assert(buffer);
	VertexBuffer *gl_buffer = static_cast<VertexBuffer *>(buffer);

	glBindBuffer(gl_buffer->data->type, gl_buffer->data->id);
	return glMapBufferRange(gl_buffer->data->type, 0, gl_buffer->data->size, GL_MAP_WRITE_BIT);
}

void Driver::unmap(backend::VertexBuffer *buffer)
{
	assert(buffer);
	VertexBuffer *gl_buffer = static_cast<VertexBuffer *>(buffer);

	glBindBuffer(gl_buffer->data->type, gl_buffer->data->id);
	glUnmapBuffer(gl_buffer->data->type);
}

void *Driver::map(backend::IndexBuffer *buffer)
{
	assert(buffer);
	IndexBuffer *gl_buffer = static_cast<IndexBuffer *>(buffer);

	glBindBuffer(gl_buffer->data->type, gl_buffer->data->id);
	return glMapBufferRange(gl_buffer->data->type, 0, gl_buffer->data->size, GL_MAP_WRITE_BIT);
}

void Driver::unmap(backend::IndexBuffer *buffer)
{
	assert(buffer);
	IndexBuffer *gl_buffer = static_cast<IndexBuffer *>(buffer);

	glBindBuffer(gl_buffer->data->type, gl_buffer->data->id);
	glUnmapBuffer(gl_buffer->data->type);
}

void *Driver::map(backend::UniformBuffer *buffer)
{
	assert(buffer);
	UniformBuffer *gl_buffer = static_cast<UniformBuffer *>(buffer);

	glBindBuffer(gl_buffer->data->type, gl_buffer->data->id);
	return glMapBufferRange(gl_buffer->data->type, 0, gl_buffer->data->size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}

void Driver::unmap(backend::UniformBuffer *buffer)
{
	assert(buffer);
	UniformBuffer *gl_buffer = static_cast<UniformBuffer *>(buffer);

	glBindBuffer(gl_buffer->data->type, gl_buffer->data->id);
	glUnmapBuffer(gl_buffer->data->type);
}

//
bool Driver::acquire(
	backend::SwapChain *swap_chain,
	uint32_t *new_image
)
{
	assert(swap_chain);
	SwapChain *gl_swap_chain = static_cast<SwapChain *>(swap_chain);
	Platform::makeCurrent(gl_swap_chain->surface);

	*new_image = gl_swap_chain->current_image;
	return true;
}

bool Driver::present(
	backend::SwapChain *swap_chain,
	uint32_t num_wait_command_buffers,
	backend::CommandBuffer * const *wait_command_buffers
)
{
	assert(swap_chain);
	SwapChain *gl_swap_chain = static_cast<SwapChain *>(swap_chain);
	Platform::makeCurrent(gl_swap_chain->surface);

	// TODO: wait for command buffers

	Platform::swapBuffers(gl_swap_chain->surface);

	gl_swap_chain->current_image++;
	gl_swap_chain->current_image %= gl_swap_chain->num_images;

	return true;
}

bool Driver::wait(
	uint32_t num_wait_command_buffers,
	backend::CommandBuffer * const *wait_command_buffers
)
{
	// TODO: implement
	return false;
}

void Driver::wait()
{
	glFinish();
}

//
void Driver::bindUniformBuffer(
	backend::BindSet *bind_set,
	uint32_t binding,
	const backend::UniformBuffer *uniform_buffer
)
{
	assert(binding < BindSet::MAX_BINDINGS);

	if (bind_set == nullptr)
		return;

	BindSet *gl_bind_set = static_cast<BindSet *>(bind_set);
	const UniformBuffer *gl_uniform_buffer = static_cast<const UniformBuffer *>(uniform_buffer);
	
	BindSet::Data &data = gl_bind_set->datas[binding];
	BindSet::DataType &type = gl_bind_set->types[binding];

	GLuint id = (gl_uniform_buffer != nullptr) ? gl_uniform_buffer->data->id : 0;
	GLsizeiptr size = (gl_uniform_buffer != nullptr) ? gl_uniform_buffer->data->size : 0;

	bool buffer_changed = (data.ubo.id != id) || (data.ubo.size != size);
	bool type_changed = (type != BindSet::DataType::UNIFORM_BUFFER);

	bitset_set(gl_bind_set->binding_used, binding, (gl_uniform_buffer != nullptr));
	bitset_set(gl_bind_set->binding_dirty, binding, type_changed || buffer_changed);

	data.ubo.id = id;
	data.ubo.size = size;
	data.ubo.offset = 0;

	type = BindSet::DataType::UNIFORM_BUFFER;
}

void Driver::bindTexture(
	backend::BindSet *bind_set,
	uint32_t binding,
	const backend::Texture *texture
)
{
	if (bind_set == nullptr)
		return;

	const Texture *gl_texture = static_cast<const Texture *>(texture);

	uint32_t num_mipmaps = (gl_texture) ? gl_texture->mips : 0;
	uint32_t num_layers = (gl_texture) ? gl_texture->layers : 0;

	bindTexture(bind_set, binding, texture, 0, num_mipmaps, 0, num_layers);
}

void Driver::bindTexture(
	backend::BindSet *bind_set,
	uint32_t binding,
	const backend::Texture *texture,
	uint32_t base_mip,
	uint32_t num_mips,
	uint32_t base_layer,
	uint32_t num_layers
)
{
	assert(binding < BindSet::MAX_BINDINGS);

	if (bind_set == nullptr)
		return;

	BindSet *gl_bind_set = static_cast<BindSet *>(bind_set);
	const Texture *gl_texture = static_cast<const Texture *>(texture);
	
	BindSet::Data &data = gl_bind_set->datas[binding];
	BindSet::DataType &type = gl_bind_set->types[binding];

	GLuint texture_id = (gl_texture != nullptr) ? gl_texture->id : 0;
	GLenum texture_type = (gl_texture != nullptr) ? gl_texture->type : GL_TEXTURE_2D;

	bool texture_mips_changed = (data.texture.base_mip != base_mip) || (data.texture.num_mips != num_mips);
	bool texture_layers_changed = (data.texture.num_layers != num_layers) || (data.texture.base_layer != base_layer);
	bool texture_id_changed = (data.texture.id != texture_id);
	bool texture_type_changed = (data.texture.type != texture_type);

	bool texture_changed = texture_id_changed || texture_type_changed || texture_mips_changed || texture_layers_changed;
	bool type_changed = (type != BindSet::DataType::TEXTURE);

	bitset_set(gl_bind_set->binding_used, binding, (gl_texture != nullptr));
	bitset_set(gl_bind_set->binding_dirty, binding, type_changed || texture_changed);

	data.texture.id = texture_id;
	data.texture.type = texture_type;
	data.texture.base_mip = base_mip;
	data.texture.num_mips = num_mips;
	data.texture.base_layer = base_layer;
	data.texture.num_layers = num_layers;

	type = BindSet::DataType::TEXTURE;
}

//
void Driver::clearPushConstants(
)
{
	push_constants_size = 0;
}

void Driver::setPushConstants(
	uint8_t size,
	const void *data
)
{
	assert(size <= MAX_PUSH_CONSTANT_SIZE);
	assert(data);

	memcpy(push_constants, data, size);
	push_constants_size = size;
}

void Driver::clearBindSets(
)
{
	pipeline_state.num_bind_sets = 0;
	memset(pipeline_state.bound_bind_sets, 0, sizeof(pipeline_state.bound_bind_sets));

	pipeline_state_overrides.bound_bind_sets = 0;
	pipeline_dirty = true;
}

void Driver::allocateBindSets(
	uint8_t size
)
{
	assert(size < MAX_BIND_SETS);

	pipeline_state.num_bind_sets = size;
	memset(pipeline_state.bound_bind_sets, 0, sizeof(pipeline_state.bound_bind_sets));

	uint16_t mask = (1 << (size + 1)) - 1;
	pipeline_state_overrides.bound_bind_sets = mask;
	pipeline_dirty = true;
}

void Driver::pushBindSet(
	backend::BindSet *bind_set
)
{
	assert(pipeline_state.num_bind_sets < MAX_BIND_SETS);
	assert(bind_set);

	BindSet *gl_bind_set = static_cast<BindSet *>(bind_set);

	uint32_t binding = pipeline_state.num_bind_sets++;
	pipeline_state.bound_bind_sets[binding] = gl_bind_set;
	pipeline_state_overrides.bound_bind_sets |= (1 << binding);
	pipeline_dirty = true;
}

void Driver::setBindSet(
	uint32_t binding,
	backend::BindSet *bind_set
)
{
	assert(binding < pipeline_state.num_bind_sets);
	assert(bind_set);

	BindSet *gl_bind_set = static_cast<BindSet *>(bind_set);

	if (pipeline_state.bound_bind_sets[binding] != gl_bind_set)
	{
		pipeline_state.bound_bind_sets[binding] = gl_bind_set;
		pipeline_state_overrides.bound_bind_sets |= (1 << binding);
		pipeline_dirty = true;
	}
}

void Driver::clearShaders(
)
{
	for (uint16_t i = 0; i < MAX_SHADERS; ++i)
	{
		GLuint id = pipeline_state.bound_shaders[i];
		if (id == 0)
			continue;

		pipeline_state.bound_shaders[i] = 0;
		pipeline_state_overrides.bound_shaders |= (1 << i);
		pipeline_dirty = true;
	}
}

void Driver::setShader(
	ShaderType type,
	const backend::Shader *shader
)
{
	const Shader *gl_shader = static_cast<const Shader *>(shader);

	if (gl_shader && gl_shader->type != Utils::getShaderType(type))
	{
		Log::error("opengl::Driver::bindShader(): shader type mismatch\n");
		return;
	}

	uint8_t index = static_cast<uint8_t>(type);
	GLuint id = (gl_shader) ? gl_shader->id : GL_NONE;

	if (pipeline_state.bound_shaders[index] != id)
	{
		pipeline_state.bound_shaders[index] = id;
		pipeline_state_overrides.bound_shaders |= (1 << index);
		pipeline_dirty = true;
	}
}

//
void Driver::setViewport(
	float x,
	float y,
	float width,
	float height
)
{
	if (pipeline_state.viewport_x != x)
	{
		pipeline_state.viewport_x = static_cast<GLint>(x);
		pipeline_state_overrides.viewport = 1;
		pipeline_dirty = true;
	}

	if (pipeline_state.viewport_y != y)
	{
		pipeline_state.viewport_y = static_cast<GLint>(y);
		pipeline_state_overrides.viewport = 1;
		pipeline_dirty = true;
	}

	if (pipeline_state.viewport_width != width)
	{
		pipeline_state.viewport_width = static_cast<GLsizei>(width);
		pipeline_state_overrides.viewport = 1;
		pipeline_dirty = true;
	}

	if (pipeline_state.viewport_height != height)
	{
		pipeline_state.viewport_height = static_cast<GLsizei>(height);
		pipeline_state_overrides.viewport = 1;
		pipeline_dirty = true;
	}
}

void Driver::setScissor(
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height
)
{
	if (pipeline_state.scissor_x != x)
	{
		pipeline_state.scissor_x = static_cast<GLint>(x);
		pipeline_state_overrides.scissor = 1;
		pipeline_dirty = true;
	}

	if (pipeline_state.scissor_y != y)
	{
		pipeline_state.scissor_y = static_cast<GLint>(y);
		pipeline_state_overrides.scissor = 1;
		pipeline_dirty = true;
	}

	if (pipeline_state.scissor_width != width)
	{
		pipeline_state.scissor_width = static_cast<GLsizei>(width);
		pipeline_state_overrides.scissor = 1;
		pipeline_dirty = true;
	}

	if (pipeline_state.scissor_height != height)
	{
		pipeline_state.scissor_height = static_cast<GLsizei>(height);
		pipeline_state_overrides.scissor = 1;
		pipeline_dirty = true;
	}
}

void Driver::setCullMode(
	CullMode mode
)
{
	GLenum cull_mode = Utils::getCullMode(mode);

	if (pipeline_state.cull_mode != cull_mode)
	{
		pipeline_state.cull_mode = cull_mode;
		pipeline_state_overrides.rasterizer_state = 1;
		pipeline_dirty = true;
	}
}

void Driver::setDepthTest(
	bool enabled
)
{
	if (pipeline_state.depth_test != enabled)
	{
		pipeline_state.depth_test = enabled;
		pipeline_state_overrides.depth_stencil_state = 1;
		pipeline_dirty = true;
	}
}

void Driver::setDepthWrite(
	bool enabled
)
{
	if (pipeline_state.depth_write != enabled)
	{
		pipeline_state.depth_write = enabled;
		pipeline_state_overrides.depth_stencil_state = 1;
		pipeline_dirty = true;
	}
}

void Driver::setDepthCompareFunc(
	DepthCompareFunc func
)
{
	GLenum gl_func = Utils::getDepthCompareFunc(func);

	if (pipeline_state.depth_compare_func != gl_func)
	{
		pipeline_state.depth_compare_func = gl_func;
		pipeline_state_overrides.depth_stencil_state = 1;
		pipeline_dirty = true;
	}
}

void Driver::setBlending(
	bool enabled
)
{
	if (pipeline_state.blend != enabled)
	{
		pipeline_state.blend = enabled;
		pipeline_state_overrides.blend_state = 1;
		pipeline_dirty = true;
	}
}

void Driver::setBlendFactors(
	BlendFactor src_factor,
	BlendFactor dest_factor
)
{
	GLenum gl_src_factor = Utils::getBlendFactor(src_factor);
	GLenum gl_dst_factor = Utils::getBlendFactor(dest_factor);

	if (pipeline_state.blend_src_factor != gl_src_factor || pipeline_state.blend_dst_factor != gl_dst_factor)
	{
		pipeline_state.blend_src_factor = gl_src_factor;
		pipeline_state.blend_dst_factor = gl_dst_factor;
		pipeline_state_overrides.blend_state = 1;
		pipeline_dirty = true;
	}
}

//
bool Driver::resetCommandBuffer(
	backend::CommandBuffer *command_buffer
)
{
	CommandBuffer *gl_command_buffer = static_cast<CommandBuffer *>(command_buffer);
	return command_buffer_reset(gl_command_buffer);
}

bool Driver::beginCommandBuffer(
	backend::CommandBuffer *command_buffer
)
{
	CommandBuffer *gl_command_buffer = static_cast<CommandBuffer *>(command_buffer);
	return command_buffer_begin(gl_command_buffer);
}

bool Driver::endCommandBuffer(
	backend::CommandBuffer *command_buffer
)
{
	CommandBuffer *gl_command_buffer = static_cast<CommandBuffer *>(command_buffer);
	return command_buffer_end(gl_command_buffer);
}

bool Driver::submit(
	backend::CommandBuffer *command_buffer
)
{
	CommandBuffer *gl_command_buffer = static_cast<CommandBuffer *>(command_buffer);
	return command_buffer_submit(gl_command_buffer);
}

bool Driver::submitSyncked(
	backend::CommandBuffer *command_buffer,
	const backend::SwapChain *wait_swap_chain
)
{
	// TODO: wait for swap chain

	CommandBuffer *gl_command_buffer = static_cast<CommandBuffer *>(command_buffer);
	return command_buffer_submit(gl_command_buffer);
}

bool Driver::submitSyncked(
	backend::CommandBuffer *command_buffer,
	uint32_t num_wait_command_buffers,
	backend::CommandBuffer * const *wait_command_buffers
)
{
	// TODO: wait for other command buffers

	CommandBuffer *gl_command_buffer = static_cast<CommandBuffer *>(command_buffer);
	return command_buffer_submit(gl_command_buffer);
}

//
void Driver::beginRenderPass(
	backend::CommandBuffer *command_buffer,
	const backend::FrameBuffer *frame_buffer,
	const RenderPassInfo *info
)
{
	assert(info);

	if (command_buffer == nullptr || frame_buffer == nullptr)
		return;

	CommandBuffer *gl_command_buffer = static_cast<CommandBuffer *>(command_buffer);
	if (gl_command_buffer->state != CommandBufferState::RECORDING)
	{
		// TODO: log error
		return;
	}

	const FrameBuffer *gl_frame_buffer = static_cast<const FrameBuffer *>(frame_buffer);

	// Emit viewport state
	{
		setViewport(0.0f, 0.0f, static_cast<float>(gl_frame_buffer->width), static_cast<float>(gl_frame_buffer->height));

		Command *command = command_buffer_emit(gl_command_buffer, CommandType::SET_VIEWPORT);
		command->viewport.x = pipeline_state.viewport_x;
		command->viewport.y = pipeline_state.viewport_y;
		command->viewport.width = pipeline_state.viewport_width;
		command->viewport.height = pipeline_state.viewport_height;
	}

	// Emit scissor state
	{
		setScissor(0, 0, gl_frame_buffer->width, gl_frame_buffer->height);

		Command *command = command_buffer_emit(gl_command_buffer, CommandType::SET_SCISSOR);
		command->scissor.x = pipeline_state.scissor_x;
		command->scissor.y = pipeline_state.scissor_y;
		command->scissor.width = pipeline_state.scissor_width;
		command->scissor.height = pipeline_state.scissor_height;
	}

	// Emit resolve FBO commands
	if (gl_frame_buffer->resolve_fbo_id != 0)
	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::BIND_FRAME_BUFFER);
		command->bind_frame_buffer.id = gl_frame_buffer->resolve_fbo_id;
		command->bind_frame_buffer.target = GL_FRAMEBUFFER;

		for (uint8_t i = 0; i < gl_frame_buffer->num_resolve_color_attachments; ++i)
		{
			uint32_t index = gl_frame_buffer->resolve_color_attachment_indices[i];

			RenderPassLoadOp load_op = info->load_ops[index];
			RenderPassClearValue clear_value = info->clear_values[index];

			if (load_op != RenderPassLoadOp::CLEAR)
				continue;

			Command *command = command_buffer_emit(gl_command_buffer, CommandType::CLEAR_COLOR_BUFFER);
			command->clear_color_buffer.buffer = i;
			command->clear_color_buffer.clear_color[0] = clear_value.color.float32[0];
			command->clear_color_buffer.clear_color[1] = clear_value.color.float32[1];
			command->clear_color_buffer.clear_color[2] = clear_value.color.float32[2];
			command->clear_color_buffer.clear_color[3] = clear_value.color.float32[3];
		}
	}

	// Emit main FBO commands
	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::BIND_FRAME_BUFFER);
		command->bind_frame_buffer.id = gl_frame_buffer->main_fbo_id;
		command->bind_frame_buffer.target = GL_FRAMEBUFFER;
	}

	for (uint8_t i = 0; i < gl_frame_buffer->num_color_attachments; ++i)
	{
		uint32_t index = gl_frame_buffer->color_attachment_indices[i];

		RenderPassLoadOp load_op = info->load_ops[index];
		RenderPassClearValue clear_value = info->clear_values[index];

		if (load_op != RenderPassLoadOp::CLEAR)
			continue;

		Command *command = command_buffer_emit(gl_command_buffer, CommandType::CLEAR_COLOR_BUFFER);
		command->clear_color_buffer.buffer = i;
		command->clear_color_buffer.clear_color[0] = clear_value.color.float32[0];
		command->clear_color_buffer.clear_color[1] = clear_value.color.float32[1];
		command->clear_color_buffer.clear_color[2] = clear_value.color.float32[2];
		command->clear_color_buffer.clear_color[3] = clear_value.color.float32[3];
	}

	if (gl_frame_buffer->depth_stencil_attachment.id != 0)
	{
		uint32_t index = gl_frame_buffer->depth_stencil_attachment_index;

		RenderPassLoadOp load_op = info->load_ops[index];
		RenderPassClearValue clear_value = info->clear_values[index];

		if (load_op == RenderPassLoadOp::CLEAR)
		{
			Command *command = command_buffer_emit(gl_command_buffer, CommandType::CLEAR_DEPTHSTENCIL_BUFFER);
			command->clear_depth_stencil_buffer.depth = clear_value.depth_stencil.depth;
			command->clear_depth_stencil_buffer.stencil = clear_value.depth_stencil.stencil;
		}
	}

	render_pass_state = {};
	render_pass_state.resolve = (gl_frame_buffer->resolve_fbo_id != 0);
	render_pass_state.src_fbo_id = gl_frame_buffer->main_fbo_id;
	render_pass_state.dst_fbo_id = gl_frame_buffer->resolve_fbo_id;
	render_pass_state.width = gl_frame_buffer->width;
	render_pass_state.height = gl_frame_buffer->height;
	render_pass_state.mask = GL_COLOR_BUFFER_BIT;
	render_pass_state.num_draw_buffers = gl_frame_buffer->num_color_attachments;

	for (uint8_t i = 0; i < gl_frame_buffer->num_color_attachments; ++i)
		render_pass_state.draw_buffers[i] = GL_COLOR_ATTACHMENT0 + i;
}

void Driver::beginRenderPass(
	backend::CommandBuffer *command_buffer,
	const backend::SwapChain *swap_chain,
	const RenderPassInfo *info
)
{
	assert(info);

	if (command_buffer == nullptr || swap_chain == nullptr)
		return;

	CommandBuffer *gl_command_buffer = static_cast<CommandBuffer *>(command_buffer);
	if (gl_command_buffer->state != CommandBufferState::RECORDING)
	{
		// TODO: log error
		return;
	}

	const SwapChain *gl_swap_chain = static_cast<const SwapChain *>(swap_chain);

	// Emit viewport state
	{
		setViewport(0.0f, 0.0f, static_cast<float>(gl_swap_chain->width), static_cast<float>(gl_swap_chain->height));

		Command *command = command_buffer_emit(gl_command_buffer, CommandType::SET_VIEWPORT);
		command->viewport.x = pipeline_state.viewport_x;
		command->viewport.y = pipeline_state.viewport_y;
		command->viewport.width = pipeline_state.viewport_width;
		command->viewport.height = pipeline_state.viewport_height;
	}

	// Emit scissor state
	{
		setScissor(0, 0, gl_swap_chain->width, gl_swap_chain->height);

		Command *command = command_buffer_emit(gl_command_buffer, CommandType::SET_SCISSOR);
		command->scissor.x = pipeline_state.scissor_x;
		command->scissor.y = pipeline_state.scissor_y;
		command->scissor.width = pipeline_state.scissor_width;
		command->scissor.height = pipeline_state.scissor_height;
	}

	// Emit resolve FBO commands
	if (gl_swap_chain->msaa_fbo_id != 0)
	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::BIND_FRAME_BUFFER);
		command->bind_frame_buffer.id = gl_swap_chain->msaa_fbo_id;
		command->bind_frame_buffer.target = GL_FRAMEBUFFER;

		uint32_t index = 1;

		RenderPassLoadOp load_op = info->load_ops[index];
		RenderPassClearValue clear_value = info->clear_values[index];

		if (load_op == RenderPassLoadOp::CLEAR)
		{
			Command *command = command_buffer_emit(gl_command_buffer, CommandType::CLEAR_COLOR_BUFFER);
			command->clear_color_buffer.buffer = 0;
			command->clear_color_buffer.clear_color[0] = clear_value.color.float32[0];
			command->clear_color_buffer.clear_color[1] = clear_value.color.float32[1];
			command->clear_color_buffer.clear_color[2] = clear_value.color.float32[2];
			command->clear_color_buffer.clear_color[3] = clear_value.color.float32[3];
		}

		index = 2;

		load_op = info->load_ops[index];
		clear_value = info->clear_values[index];

		if (load_op == RenderPassLoadOp::CLEAR)
		{
			Command *command = command_buffer_emit(gl_command_buffer, CommandType::CLEAR_DEPTHSTENCIL_BUFFER);
			command->clear_depth_stencil_buffer.depth = clear_value.depth_stencil.depth;
			command->clear_depth_stencil_buffer.stencil = clear_value.depth_stencil.stencil;
		}
	}

	// Emit main FBO commands
	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::BIND_FRAME_BUFFER);
		command->bind_frame_buffer.id = 0;
		command->bind_frame_buffer.target = GL_FRAMEBUFFER;
	}

	uint32_t index = 0;

	RenderPassLoadOp load_op = info->load_ops[index];
	RenderPassClearValue clear_value = info->clear_values[index];

	if (load_op == RenderPassLoadOp::CLEAR)
	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::CLEAR_COLOR_BUFFER);
		command->clear_color_buffer.buffer = 0;
		command->clear_color_buffer.clear_color[0] = clear_value.color.float32[0];
		command->clear_color_buffer.clear_color[1] = clear_value.color.float32[1];
		command->clear_color_buffer.clear_color[2] = clear_value.color.float32[2];
		command->clear_color_buffer.clear_color[3] = clear_value.color.float32[3];
	}

	render_pass_state = {};
	render_pass_state.resolve = (gl_swap_chain->msaa_fbo_id != 0);
	render_pass_state.src_fbo_id = gl_swap_chain->msaa_fbo_id;
	render_pass_state.dst_fbo_id = 0;
	render_pass_state.width = gl_swap_chain->width;
	render_pass_state.height = gl_swap_chain->height;
	render_pass_state.mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	render_pass_state.num_draw_buffers = 1;
	render_pass_state.draw_buffers[0] = GL_BACK;
}

void Driver::endRenderPass(
	backend::CommandBuffer *command_buffer
)
{
	if (command_buffer == nullptr)
		return;

	CommandBuffer *gl_command_buffer = static_cast<CommandBuffer *>(command_buffer);
	if (gl_command_buffer->state != CommandBufferState::RECORDING)
	{
		// TODO: log error
		return;
	}

	if (render_pass_state.resolve)
	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::BLIT_FRAME_BUFFER);
		command->blit_frame_buffer.src_fbo_id = render_pass_state.src_fbo_id;
		command->blit_frame_buffer.dst_fbo_id = render_pass_state.dst_fbo_id;
		command->blit_frame_buffer.width = render_pass_state.width;
		command->blit_frame_buffer.height = render_pass_state.height;
		command->blit_frame_buffer.mask = render_pass_state.mask;
		command->blit_frame_buffer.num_draw_buffers = render_pass_state.num_draw_buffers;
		memcpy(command->blit_frame_buffer.draw_buffers, render_pass_state.draw_buffers, sizeof(GLenum) * render_pass_state.num_draw_buffers);
	}
}

void Driver::drawIndexedPrimitive(
	backend::CommandBuffer *command_buffer,
	const RenderPrimitive *render_primitive
)
{
	assert(render_primitive);

	if (command_buffer == nullptr)
		return;

	CommandBuffer *gl_command_buffer = static_cast<CommandBuffer *>(command_buffer);
	if (gl_command_buffer->state != CommandBufferState::RECORDING)
	{
		// TODO: log error
		return;
	}

	flushPipelineState(gl_command_buffer);

	if (push_constants_size > 0)
	{
		uint32_t new_offset = gl_command_buffer->push_constants_offset + push_constants_size;

		uint32_t padding = push_constants_size % uniform_buffer_offset_alignment;
		if (padding > 0)
			padding = uniform_buffer_offset_alignment - padding;

		new_offset += padding;

		assert(new_offset < CommandBuffer::MAX_PUSH_CONSTANT_BUFFER_SIZE);

		uint8_t *buffer = reinterpret_cast<uint8_t *>(gl_command_buffer->push_constants_mapped_memory);
		buffer += gl_command_buffer->push_constants_offset;

		memcpy(buffer, push_constants, push_constants_size);

		Command *command = command_buffer_emit(gl_command_buffer, CommandType::BIND_UNIFORM_BUFFER);
		command->bind_uniform_buffer.binding = 0;
		command->bind_uniform_buffer.id = gl_command_buffer->push_constants->id;
		command->bind_uniform_buffer.offset = gl_command_buffer->push_constants_offset;
		command->bind_uniform_buffer.size = push_constants_size;

		gl_command_buffer->push_constants_offset = new_offset;
	}

	const VertexBuffer *vb = static_cast<const VertexBuffer *>(render_primitive->vertices);
	const IndexBuffer *ib = static_cast<const IndexBuffer *>(render_primitive->indices);

	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::BIND_VERTEX_BUFFER);
		command->bind_vertex_buffer.vao_id = vb->vao_id;
	}
	
	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::BIND_INDEX_BUFFER);
		command->bind_index_buffer.id = ib->data->id;
	}

	{
		Command *command = command_buffer_emit(gl_command_buffer, CommandType::DRAW_INDEXED_PRIMITIVE);
		command->draw_indexed_primitive.primitive_type = Utils::getPrimitiveType(render_primitive->type);
		command->draw_indexed_primitive.index_format = ib->index_format;
		command->draw_indexed_primitive.num_indices = ib->num_indices;
		command->draw_indexed_primitive.base_index = render_primitive->base_index;
		command->draw_indexed_primitive.base_vertex = render_primitive->vertex_index_offset;
		command->draw_indexed_primitive.num_instances = 1;
	}
}
}
