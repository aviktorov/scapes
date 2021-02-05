#include "ImGuiRenderer.h"

#include <render/SwapChain.h>
#include <render/Texture.h>
#include <render/Shader.h>

#include "imgui.h"
#include <string>

using namespace render;
using namespace render::backend;
using namespace render::shaders;

/*
 */
static std::string vertex_shader_source =
"#version 450 core\n"
"#pragma shader_stage(vertex)\n"
"layout(location = 0) in vec2 aPos;\n"
"layout(location = 1) in vec2 aUV;\n"
"layout(location = 2) in vec4 aColor;\n"
"layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;\n"
"\n"
"out gl_PerVertex { vec4 gl_Position; };\n"
"layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;\n"
"\n"
"void main()\n"
"{\n"
"    Out.Color = aColor;\n"
"    Out.UV = aUV;\n"
"    gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);\n"
"}\n";

static std::string fragment_shader_source = 
"#version 450 core\n"
"#pragma shader_stage(fragment)\n"
"layout(location = 0) out vec4 fColor;\n"
"layout(set=0, binding=0) uniform sampler2D sTexture;\n"
"layout(location = 0) in struct { vec4 Color; vec2 UV; } In;\n"
"void main()\n"
"{\n"
"    fColor = In.Color * texture(sTexture, In.UV.st);\n"
"}\n";

/*
 */
ImGuiRenderer::ImGuiRenderer(Driver *driver, Compiler *compiler)
	: driver(driver), compiler(compiler)
{

}

ImGuiRenderer::~ImGuiRenderer()
{
	shutdown();
}

/*
 */
void ImGuiRenderer::init(ImGuiContext *context)
{
	ImGui::SetCurrentContext(context);

	uint32_t width, height;
	unsigned char *pixels = nullptr;

	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->GetTexDataAsRGBA32(&pixels, reinterpret_cast<int *>(&width), reinterpret_cast<int *>(&height));

	vertex_shader = new render::Shader(driver, compiler);
	vertex_shader->compileFromMemory(ShaderType::VERTEX, static_cast<uint32_t>(vertex_shader_source.size()), vertex_shader_source.c_str());

	fragment_shader = new render::Shader(driver, compiler);
	fragment_shader->compileFromMemory(ShaderType::FRAGMENT, static_cast<uint32_t>(fragment_shader_source.size()), fragment_shader_source.c_str());

	font_texture = driver->createTexture2D(width, height, 1, Format::R8G8B8A8_UNORM, Multisample::COUNT_1, pixels);
	font_bind_set = driver->createBindSet();
	driver->bindTexture(font_bind_set, 0, font_texture);

	io.Fonts->TexID = reinterpret_cast<ImTextureID>(font_bind_set);
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendRendererName = "Scape ImGui";
}

void ImGuiRenderer::shutdown()
{
	driver->destroyVertexBuffer(vertices);
	vertices = nullptr;

	driver->destroyIndexBuffer(indices);
	indices = nullptr;

	driver->destroyTexture(font_texture);
	font_texture = nullptr;

	driver->destroyBindSet(font_bind_set);
	font_bind_set = nullptr;

	invalidateTextureIDs();

	delete vertex_shader;
	vertex_shader = nullptr;

	delete fragment_shader;
	fragment_shader = nullptr;
}

/*
 */
ImTextureID ImGuiRenderer::fetchTextureID(const backend::Texture *texture)
{
	auto it = registered_textures.find(texture);
	if (it != registered_textures.end())
		return it->second;

	BindSet *bind_set = driver->createBindSet();
	driver->bindTexture(bind_set, 0, texture);

	registered_textures.insert(std::make_pair(texture, bind_set));

	return reinterpret_cast<ImTextureID>(bind_set);
}

void ImGuiRenderer::invalidateTextureIDs()
{
	for (auto &it : registered_textures)
		driver->destroyBindSet(it.second);

	registered_textures.clear();
}

/*
 */
void ImGuiRenderer::updateBuffers(const ImDrawData *draw_data)
{
	uint32_t num_indices = draw_data->TotalIdxCount;
	uint32_t num_vertices = draw_data->TotalVtxCount;

	if (num_vertices == 0 || num_indices == 0)
		return;

	size_t index_size = sizeof(ImDrawIdx);
	uint16_t vertex_size = static_cast<uint16_t>(sizeof(ImDrawVert));

	assert(index_size == 2 || index_size == 4);

	backend::IndexFormat index_format = backend::IndexFormat::UINT16;
	if (index_size == 4)
		index_format = backend::IndexFormat::UINT32;

	static const uint8_t num_attributes = 3;
	static VertexAttribute attributes[] =
	{
		{ backend::Format::R32G32_SFLOAT, offsetof(ImDrawVert, pos), },
		{ backend::Format::R32G32_SFLOAT, offsetof(ImDrawVert, uv), },
		{ backend::Format::R8G8B8A8_UNORM, offsetof(ImDrawVert, col), },
	};

	// resize index buffer
	if (index_buffer_size < index_size * num_indices)
	{
		index_buffer_size = index_size * num_indices;

		driver->destroyIndexBuffer(indices);
		indices = driver->createIndexBuffer(BufferType::DYNAMIC, index_format, num_indices, nullptr);
	}

	// resize vertex buffer
	if (vertex_buffer_size < vertex_size * num_vertices)
	{
		vertex_buffer_size = vertex_size * num_vertices;

		driver->destroyVertexBuffer(vertices);
		vertices = driver->createVertexBuffer(BufferType::DYNAMIC, vertex_size, num_vertices, num_attributes, attributes, nullptr);
	}

	ImDrawVert *vertex_data = reinterpret_cast<ImDrawVert *>(driver->map(vertices));
	ImDrawIdx *index_data = reinterpret_cast<ImDrawIdx *>(driver->map(indices));

	for (int i = 0; i < draw_data->CmdListsCount; ++i)
	{
		const ImDrawList *commands = draw_data->CmdLists[i];
		memcpy(vertex_data, commands->VtxBuffer.Data, commands->VtxBuffer.Size * vertex_size);
		memcpy(index_data, commands->IdxBuffer.Data, commands->IdxBuffer.Size * index_size);
		vertex_data += commands->VtxBuffer.Size;
		index_data += commands->IdxBuffer.Size;
	}

	driver->unmap(vertices);
	driver->unmap(indices);
}

void ImGuiRenderer::setupRenderState(const render::RenderFrame &frame, const ImDrawData *draw_data)
{
	driver->clearBindSets();
	driver->allocateBindSets(1);

	driver->clearShaders();
	driver->setShader(ShaderType::VERTEX, vertex_shader->getBackend());
	driver->setShader(ShaderType::FRAGMENT, fragment_shader->getBackend());

	float transform[4];
	transform[0] = 2.0f / draw_data->DisplaySize.x;
	transform[1] = 2.0f / draw_data->DisplaySize.y;
	transform[2] = -1.0f - draw_data->DisplayPos.x * transform[0];
	transform[3] = -1.0f - draw_data->DisplayPos.y * transform[1];
	uint8_t size = static_cast<uint8_t>(sizeof(float) * 4);

	driver->clearPushConstants();
	driver->setPushConstants(size, transform);

	driver->setBlending(true);
	driver->setBlendFactors(BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_ALPHA);
	driver->setCullMode(CullMode::NONE);
	driver->setDepthWrite(false);
	driver->setDepthTest(false);
}

void ImGuiRenderer::render(const render::RenderFrame &frame)
{
	const ImDrawData *draw_data = ImGui::GetDrawData();
	const ImVec2 &clip_offset = draw_data->DisplayPos;
	const ImVec2 &clip_scale = draw_data->FramebufferScale;

	updateBuffers(draw_data);
	setupRenderState(frame, draw_data);

	backend::RenderPrimitive primitive;
	primitive.type = backend::RenderPrimitiveType::TRIANGLE_LIST;
	primitive.vertices = vertices;
	primitive.indices = indices;

	uint32_t index_offset = 0;
	int32_t vertex_index_offset = 0;

	for (int i = 0; i < draw_data->CmdListsCount; ++i)
	{
		const ImDrawList *list = draw_data->CmdLists[i];
		const ImDrawCmd *commands = list->CmdBuffer.Data;

		for (int j = 0; j < list->CmdBuffer.Size; ++j)
		{
			const ImDrawCmd &command = commands[j];
			if (command.UserCallback)
			{
				if (command.UserCallback == ImDrawCallback_ResetRenderState)
					setupRenderState(frame, draw_data);
				else
					command.UserCallback(list, &command);
			}
			else
			{
				primitive.num_indices = command.ElemCount;
				primitive.base_index = index_offset + command.IdxOffset;
				primitive.vertex_index_offset = vertex_index_offset + command.VtxOffset;

				backend::BindSet *bind_set = reinterpret_cast<backend::BindSet *>(command.TextureId);
				driver->setBindSet(0, bind_set);

				float x0 = (command.ClipRect.x - clip_offset.x) * clip_scale.x;
				float y0 = (command.ClipRect.y - clip_offset.y) * clip_scale.y;
				float x1 = (command.ClipRect.z - clip_offset.x) * clip_scale.x;
				float y1 = (command.ClipRect.w - clip_offset.x) * clip_scale.y;
				
				x0 = std::max(0.0f, x0);
				y0 = std::max(0.0f, y0);

				uint32_t width = static_cast<uint32_t>(x1 - x0);
				uint32_t height = static_cast<uint32_t>(y1 - y0);

				driver->setScissor(static_cast<int32_t>(x0), static_cast<int32_t>(y0), width, height);
				driver->drawIndexedPrimitive(frame.command_buffer, &primitive);
			}
		}

		index_offset += list->IdxBuffer.Size;
		vertex_index_offset += list->VtxBuffer.Size;
	}
}
