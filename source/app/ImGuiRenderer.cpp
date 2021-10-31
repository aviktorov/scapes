#include "ImGuiRenderer.h"

#include <scapes/visual/API.h>
#include <scapes/visual/Resources.h>

#include <scapes/foundation/math/Math.h>

#include "imgui.h"
#include <string>

/*
 */
static std::string vertex_shader_source =
"#version 450 core\n"
"#pragma shader_stage(vertex)\n"
"layout(location = 0) in vec2 aPos;\n"
"layout(location = 1) in vec2 aUV;\n"
"layout(location = 2) in vec4 aColor;\n"
"layout(push_constant) uniform uPushConstant { mat4 uProjection; } pc;\n"
"\n"
"out gl_PerVertex { vec4 gl_Position; };\n"
"layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;\n"
"\n"
"void main()\n"
"{\n"
"    Out.Color = aColor;\n"
"    Out.UV = aUV;\n"
"    gl_Position = pc.uProjection * vec4(aPos, 0.0f, 1.0f);\n"
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
ImGuiRenderer::ImGuiRenderer(scapes::foundation::render::Device *device, scapes::visual::API *visual_api)
	: device(device), visual_api(visual_api)
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

	pipeline_state = device->createPipelineState();

	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->GetTexDataAsRGBA32(&pixels, reinterpret_cast<int *>(&width), reinterpret_cast<int *>(&height));

	vertex_shader = visual_api->loadShaderFromMemory(
		reinterpret_cast<const uint8_t *>(vertex_shader_source.c_str()),
		static_cast<uint32_t>(vertex_shader_source.size()),
		scapes::foundation::render::ShaderType::VERTEX
	);

	fragment_shader = visual_api->loadShaderFromMemory(
		reinterpret_cast<const uint8_t *>(fragment_shader_source.c_str()),
		static_cast<uint32_t>(fragment_shader_source.size()),
		scapes::foundation::render::ShaderType::FRAGMENT
	);

	font_texture = visual_api->createTexture2D(
		scapes::foundation::render::Format::R8G8B8A8_UNORM,
		width,
		height,
		1,
		pixels
	);

	font_bind_set = device->createBindSet();
	device->bindTexture(font_bind_set, 0, font_texture->gpu_data);

	io.Fonts->TexID = reinterpret_cast<ImTextureID>(font_bind_set);
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendRendererName = "Scape ImGui";
}

void ImGuiRenderer::shutdown()
{
	device->destroyVertexBuffer(vertices);
	vertices = nullptr;

	device->destroyIndexBuffer(indices);
	indices = nullptr;

	device->destroyBindSet(font_bind_set);
	font_bind_set = nullptr;

	device->destroyPipelineState(pipeline_state);
	pipeline_state = nullptr;

	invalidateTextureIDs();
}

/*
 */
ImTextureID ImGuiRenderer::fetchTextureID(const scapes::foundation::render::Texture *texture)
{
	auto it = registered_textures.find(texture);
	if (it != registered_textures.end())
		return it->second;

	scapes::foundation::render::BindSet *bind_set = device->createBindSet();
	device->bindTexture(bind_set, 0, texture);

	registered_textures.insert(std::make_pair(texture, bind_set));

	return reinterpret_cast<ImTextureID>(bind_set);
}

void ImGuiRenderer::invalidateTextureIDs()
{
	for (auto &it : registered_textures)
		device->destroyBindSet(it.second);

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

	constexpr size_t index_size = sizeof(ImDrawIdx);
	constexpr uint16_t vertex_size = static_cast<uint16_t>(sizeof(ImDrawVert));

	static_assert(index_size == 2 || index_size == 4, "Wrong ImDrawIdx size");
	static_assert(vertex_size == 20, "Wrong ImDrawVert size");

	scapes::foundation::render::IndexFormat index_format = scapes::foundation::render::IndexFormat::UINT16;
	if (index_size == 4)
		index_format = scapes::foundation::render::IndexFormat::UINT32;

	static const uint8_t num_attributes = 3;
	static scapes::foundation::render::VertexAttribute attributes[3] =
	{
		{ scapes::foundation::render::Format::R32G32_SFLOAT, offsetof(ImDrawVert, pos), },
		{ scapes::foundation::render::Format::R32G32_SFLOAT, offsetof(ImDrawVert, uv), },
		{ scapes::foundation::render::Format::R8G8B8A8_UNORM, offsetof(ImDrawVert, col), },
	};

	// resize index buffer
	if (index_buffer_size < index_size * num_indices)
	{
		index_buffer_size = index_size * num_indices;

		device->destroyIndexBuffer(indices);
		indices = device->createIndexBuffer(scapes::foundation::render::BufferType::DYNAMIC, index_format, num_indices, nullptr);
	}

	// resize vertex buffer
	if (vertex_buffer_size < vertex_size * num_vertices)
	{
		vertex_buffer_size = vertex_size * num_vertices;

		device->destroyVertexBuffer(vertices);
		vertices = device->createVertexBuffer(scapes::foundation::render::BufferType::DYNAMIC, vertex_size, num_vertices, num_attributes, attributes, nullptr);
	}

	ImDrawVert *vertex_data = reinterpret_cast<ImDrawVert *>(device->map(vertices));
	ImDrawIdx *index_data = reinterpret_cast<ImDrawIdx *>(device->map(indices));

	for (int i = 0; i < draw_data->CmdListsCount; ++i)
	{
		const ImDrawList *commands = draw_data->CmdLists[i];
		memcpy(vertex_data, commands->VtxBuffer.Data, commands->VtxBuffer.Size * vertex_size);
		memcpy(index_data, commands->IdxBuffer.Data, commands->IdxBuffer.Size * index_size);
		vertex_data += commands->VtxBuffer.Size;
		index_data += commands->IdxBuffer.Size;
	}

	device->unmap(vertices);
	device->unmap(indices);
}

void ImGuiRenderer::setupRenderState(const ImDrawData *draw_data)
{
	device->setVertexStream(pipeline_state, 0, vertices);

	device->setShader(pipeline_state, scapes::foundation::render::ShaderType::VERTEX, vertex_shader.get()->shader);
	device->setShader(pipeline_state, scapes::foundation::render::ShaderType::FRAGMENT, fragment_shader.get()->shader);

	float L = draw_data->DisplayPos.x;
	float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
	float T = draw_data->DisplayPos.y;
	float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

	if (!device->isFlipped())
		std::swap(T,B);

	scapes::foundation::math::mat4 projection = scapes::foundation::math::ortho(L, R, B, T, 0.0f, 1.0f);

	device->setPushConstants(pipeline_state, sizeof(scapes::foundation::math::mat4), &projection);

	device->setBlending(pipeline_state, true);
	device->setBlendFactors(pipeline_state, scapes::foundation::render::BlendFactor::SRC_ALPHA, scapes::foundation::render::BlendFactor::ONE_MINUS_SRC_ALPHA);
	device->setCullMode(pipeline_state, scapes::foundation::render::CullMode::NONE);
	device->setDepthWrite(pipeline_state, false);
	device->setDepthTest(pipeline_state, false);
}

void ImGuiRenderer::render(scapes::foundation::render::CommandBuffer *command_buffer)
{
	const ImDrawData *draw_data = ImGui::GetDrawData();
	const ImVec2 &clip_offset = draw_data->DisplayPos;
	const ImVec2 &clip_scale = draw_data->FramebufferScale;

	ImVec2 fb_size(draw_data->DisplaySize.x * clip_scale.x, draw_data->DisplaySize.y * clip_scale.y); 

	updateBuffers(draw_data);
	setupRenderState(draw_data);

	uint32_t index_offset = 0;
	int32_t vertex_offset = 0;

	device->setViewport(pipeline_state, 0, 0, static_cast<uint32_t>(fb_size.x), static_cast<uint32_t>(fb_size.y));

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
					setupRenderState(draw_data);
				else
					command.UserCallback(list, &command);
			}
			else
			{
				uint32_t num_indices = command.ElemCount;
				uint32_t base_index = index_offset + command.IdxOffset;
				int32_t base_vertex = vertex_offset + command.VtxOffset;

				scapes::foundation::render::BindSet *bind_set = reinterpret_cast<scapes::foundation::render::BindSet *>(command.TextureId);
				device->setBindSet(pipeline_state, 0, bind_set);

				float x0 = (command.ClipRect.x - clip_offset.x) * clip_scale.x;
				float y0 = (command.ClipRect.y - clip_offset.y) * clip_scale.y;
				float x1 = (command.ClipRect.z - clip_offset.x) * clip_scale.x;
				float y1 = (command.ClipRect.w - clip_offset.y) * clip_scale.y;
				
				if (device->isFlipped())
				{
					y0 = fb_size.y - y0;
					y1 = fb_size.y - y1;
					std::swap(y1, y0);
				}

				x0 = std::max(0.0f, x0);
				y0 = std::max(0.0f, y0);

				uint32_t width = static_cast<uint32_t>(x1 - x0);
				uint32_t height = static_cast<uint32_t>(y1 - y0);

				device->setScissor(pipeline_state, static_cast<int32_t>(x0), static_cast<int32_t>(y0), width, height);
				device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, indices, num_indices, base_index, base_vertex, 1, 0);
			}
		}

		index_offset += list->IdxBuffer.Size;
		vertex_offset += list->VtxBuffer.Size;
	}
}
