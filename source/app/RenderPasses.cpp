#include "RenderPasses.h"

#include <scapes/foundation/game/World.h>
#include <scapes/foundation/game/Query.h>

#include <scapes/foundation/components/Components.h>
#include <scapes/visual/Components.h>

#include <imgui.h>
#include <imgui_internal.h>

using namespace scapes;

/*
 */
RenderPassGraphicsBase::RenderPassGraphicsBase()
{

}

RenderPassGraphicsBase::~RenderPassGraphicsBase()
{
	clear();
}

/*
 */
void RenderPassGraphicsBase::init()
{
	assert(render_graph);

	device = render_graph->getDevice();
	world = render_graph->getWorld();

	pipeline_state = device->createPipelineState();

	onInit();
}

void RenderPassGraphicsBase::shutdown()
{
	onShutdown();

	clear();
}

void RenderPassGraphicsBase::render(foundation::render::CommandBuffer *command_buffer)
{
	if (!canRender())
		return;

	// TODO: cache this?
	device->clearBindSets(pipeline_state);

	uint8_t current_binding = 0;
	for (size_t i = 0; i < input_groups.size(); ++i)
	{
		const char *group_name = input_groups[i].c_str();
		foundation::render::BindSet *bindings = render_graph->getGroupBindings(group_name);
		device->setBindSet(pipeline_state, current_binding++, bindings);
	}

	for (size_t i = 0; i < input_render_buffers.size(); ++i)
	{
		const char *texture_name = input_render_buffers[i].c_str();
		foundation::render::BindSet *bindings = render_graph->getRenderBufferBindings(texture_name);
		device->setBindSet(pipeline_state, current_binding++, bindings);
	}

	if (render_pass_swapchain)
	{
		foundation::render::SwapChain *swap_chain = render_graph->getSwapChain();
		assert(swap_chain);

		device->beginRenderPass(command_buffer, render_pass_swapchain, swap_chain);
		onRender(command_buffer);
		device->endRenderPass(command_buffer);
	}

	if (render_pass_offscreen)
	{
		// TODO: cache this?
		const char *render_buffers[32];
		uint32_t num_render_buffers = 0;

		for (size_t i = 0; i < color_outputs.size(); ++i)
			render_buffers[num_render_buffers++] = color_outputs[i].texture_name.c_str();

		if (has_depthstencil_output)
			render_buffers[num_render_buffers++] = depthstencil_output.texture_name.c_str();

		foundation::render::FrameBuffer *frame_buffer = render_graph->fetchFrameBuffer(num_render_buffers, render_buffers);
		assert(frame_buffer);

		device->beginRenderPass(command_buffer, render_pass_offscreen, frame_buffer);
		onRender(command_buffer);
		device->endRenderPass(command_buffer);
	}
}

void RenderPassGraphicsBase::invalidate()
{
	uint32_t width = render_graph->getWidth();
	uint32_t height = render_graph->getHeight();

	device->setViewport(pipeline_state, 0, 0, width, height);
	device->setScissor(pipeline_state, 0, 0, width, height);

	device->destroyRenderPass(render_pass_swapchain);
	render_pass_swapchain = nullptr;

	device->destroyRenderPass(render_pass_offscreen);
	render_pass_offscreen = nullptr;

	createRenderPassSwapChain();
	createRenderPassOffscreen();

	onInvalidate();
}

/*
 */
void RenderPassGraphicsBase::addInputGroup(const char *name)
{
	input_groups.push_back(std::string(name));
}

void RenderPassGraphicsBase::removeInputGroup(const char *name)
{
	const std::string group_name = std::string(name);
	auto it = std::find(input_groups.begin(), input_groups.end(), group_name);
	if (it == input_groups.end())
		return;

	input_groups.erase(it);
}

void RenderPassGraphicsBase::removeAllInputGroups()
{
	input_groups.clear();
}

/*
 */
void RenderPassGraphicsBase::addInputRenderBuffer(const char *name)
{
	input_render_buffers.push_back(std::string(name));
}

void RenderPassGraphicsBase::removeInputRenderBuffer(const char *name)
{
	const std::string texture_name = std::string(name);
	auto it = std::find(input_render_buffers.begin(), input_render_buffers.end(), texture_name);
	if (it == input_render_buffers.end())
		return;

	input_render_buffers.erase(it);
}

void RenderPassGraphicsBase::removeAllInputRenderBuffers()
{
	input_render_buffers.clear();
}

/*
 */
void RenderPassGraphicsBase::addColorOutput(
	const char *name,
	foundation::render::RenderPassLoadOp load_op,
	foundation::render::RenderPassStoreOp store_op,
	foundation::render::RenderPassClearColor clear_value
)
{
	FrameBufferOutput output = {};
	output.texture_name = std::string(name);
	output.load_op = load_op;
	output.store_op = store_op;
	output.clear_value.as_color = clear_value;

	color_outputs.push_back(output);
}

void RenderPassGraphicsBase::removeColorOutput(const char *name)
{
	const std::string texture_name = std::string(name);
	for (auto it = color_outputs.begin(); it != color_outputs.end(); ++it)
	{
		if (it->texture_name == texture_name)
		{
			color_outputs.erase(it);
			return;
		}
	}
}

void RenderPassGraphicsBase::removeAllColorOutputs()
{
	color_outputs.clear();
}

/*
 */
void RenderPassGraphicsBase::setDepthStencilOutput(
	const char *name,
	foundation::render::RenderPassLoadOp load_op,
	foundation::render::RenderPassStoreOp store_op,
	foundation::render::RenderPassClearDepthStencil clear_value
)
{
	depthstencil_output.texture_name = std::string(name);
	depthstencil_output.load_op = load_op;
	depthstencil_output.store_op = store_op;
	depthstencil_output.clear_value.as_depth_stencil = clear_value;

	has_depthstencil_output = true;
}

void RenderPassGraphicsBase::removeDepthStencilOutput()
{
	depthstencil_output = {};
	has_depthstencil_output = false;
}

/*
 */
void RenderPassGraphicsBase::setSwapChainOutput(
	foundation::render::RenderPassLoadOp load_op,
	foundation::render::RenderPassStoreOp store_op,
	foundation::render::RenderPassClearColor clear_value
)
{
	swapchain_output.load_op = load_op;
	swapchain_output.store_op = store_op;
	swapchain_output.clear_value = clear_value;

	has_swapchain_output = true;
}

void RenderPassGraphicsBase::removeSwapChainOutput()
{
	swapchain_output = {};
	has_swapchain_output = false;
}

/*
 */
void RenderPassGraphicsBase::clear()
{
	device->destroyRenderPass(render_pass_offscreen);
	render_pass_offscreen = nullptr;

	device->destroyRenderPass(render_pass_swapchain);
	render_pass_swapchain = nullptr;

	device->destroyPipelineState(pipeline_state);
	pipeline_state = nullptr;

	removeAllInputGroups();
	removeAllInputRenderBuffers();
	removeAllColorOutputs();
	removeDepthStencilOutput();
	removeSwapChainOutput();
}

/*
 */
void RenderPassGraphicsBase::createRenderPassOffscreen()
{
	if (color_outputs.size() == 0 && !has_depthstencil_output)
		return;

	foundation::render::RenderPassAttachment attachments[32];
	uint32_t color_references[32];
	uint32_t depthstencil_reference = 0;

	foundation::render::RenderPassDescription description;

	description.resolve_attachments = nullptr;
	description.color_attachments = color_references;
	description.num_color_attachments = static_cast<uint32_t>(color_outputs.size());

	uint32_t num_attachments = 0;
	for (; num_attachments < color_outputs.size(); ++num_attachments)
	{
		foundation::render::RenderPassAttachment &attachment = attachments[num_attachments];
		const FrameBufferOutput &output = color_outputs[num_attachments];
		const std::string &texture_name = output.texture_name;

		attachment.clear_value = output.clear_value;
		attachment.load_op = output.load_op;
		attachment.store_op = output.store_op;
		attachment.samples = foundation::render::Multisample::COUNT_1;
		attachment.format = render_graph->getRenderBufferFormat(texture_name.c_str());

		color_references[num_attachments] = num_attachments;
	}

	if (has_depthstencil_output)
	{
		foundation::render::RenderPassAttachment &attachment = attachments[num_attachments];
		const std::string &texture_name = depthstencil_output.texture_name;

		attachment.clear_value = depthstencil_output.clear_value;
		attachment.load_op = depthstencil_output.load_op;
		attachment.store_op = depthstencil_output.store_op;
		attachment.samples = foundation::render::Multisample::COUNT_1;
		attachment.format = render_graph->getRenderBufferFormat(texture_name.c_str());

		depthstencil_reference = num_attachments;
		description.depthstencil_attachment = &depthstencil_reference;

		++num_attachments;
	}

	render_pass_offscreen = device->createRenderPass(num_attachments, attachments, description);
}

void RenderPassGraphicsBase::createRenderPassSwapChain()
{
	if (!has_swapchain_output)
		return;

	foundation::render::SwapChain *swap_chain = render_graph->getSwapChain();
	assert(swap_chain);

	render_pass_swapchain = device->createRenderPass(swap_chain, swapchain_output.load_op, swapchain_output.store_op, &swapchain_output.clear_value);
}

/*
 */
visual::IRenderPass *RenderPassPrepareOld::create(visual::RenderGraph *render_graph)
{
	RenderPassPrepareOld *result = new RenderPassPrepareOld();
	result->setRenderGraph(render_graph);

	return result;
}

/*
 */
void RenderPassPrepareOld::onInit()
{
	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, fullscreen_quad_vertex_shader->type, fullscreen_quad_vertex_shader->shader);

	device->clearVertexStreams(pipeline_state);
	device->setVertexStream(pipeline_state, 0, fullscreen_quad_mesh->vertex_buffer);
}

void RenderPassPrepareOld::onInvalidate()
{
	first_frame = true;
}

void RenderPassPrepareOld::onRender(foundation::render::CommandBuffer *command_buffer)
{
	device->drawIndexedPrimitiveInstanced(
		command_buffer,
		pipeline_state,
		fullscreen_quad_mesh->index_buffer,
		fullscreen_quad_mesh->num_indices
	);

	first_frame = false;
}

/*
 */
visual::IRenderPass *RenderPassGeometry::create(visual::RenderGraph *render_graph)
{
	RenderPassGeometry *result = new RenderPassGeometry();
	result->setRenderGraph(render_graph);

	return result;
}

/*
 */
void RenderPassGeometry::onInit()
{
	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, vertex_shader->type, vertex_shader->shader);
	device->setShader(pipeline_state, fragment_shader->type, fragment_shader->shader);

	device->setCullMode(pipeline_state, foundation::render::CullMode::BACK);
	device->setDepthTest(pipeline_state, true);
	device->setDepthWrite(pipeline_state, true);
}

void RenderPassGeometry::onRender(foundation::render::CommandBuffer *command_buffer)
{
	auto query = foundation::game::Query<foundation::components::Transform, visual::components::Renderable>(world);

	query.begin();

	while (query.next())
	{
		uint32_t num_items = query.getNumComponents();
		foundation::components::Transform *transforms = query.getComponents<foundation::components::Transform>(0);
		visual::components::Renderable *renderables = query.getComponents<visual::components::Renderable>(1);

		for (uint32_t i = 0; i < num_items; ++i)
		{
			const foundation::components::Transform &transform = transforms[i];
			const visual::components::Renderable &renderable = renderables[i];
			const foundation::math::mat4 &node_transform = transform.transform;

			foundation::render::BindSet *material_bindings = renderable.material->bindings;

			device->clearVertexStreams(pipeline_state);
			device->setVertexStream(pipeline_state, 0, renderable.mesh->vertex_buffer);

			device->setBindSet(pipeline_state, material_binding, material_bindings);
			device->setPushConstants(pipeline_state, static_cast<uint8_t>(sizeof(foundation::math::mat4)), &node_transform);

			device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, renderable.mesh->index_buffer, renderable.mesh->num_indices);
		}
	}
}

/*
 */
visual::IRenderPass *RenderPassLBuffer::create(visual::RenderGraph *render_graph)
{
	RenderPassLBuffer *result = new RenderPassLBuffer();
	result->setRenderGraph(render_graph);

	return result;
}

/*
 */
void RenderPassLBuffer::onRender(foundation::render::CommandBuffer *command_buffer)
{
	auto query = foundation::game::Query<visual::components::SkyLight>(world);

	query.begin();

	while (query.next())
	{
		uint32_t num_items = query.getNumComponents();
		visual::components::SkyLight *skylights = query.getComponents<visual::components::SkyLight>(0);

		for (uint32_t i = 0; i < num_items; ++i)
		{
			const visual::components::SkyLight &light = skylights[i];

			device->clearShaders(pipeline_state);
			device->setShader(pipeline_state, light.vertex_shader->type, light.vertex_shader->shader);
			device->setShader(pipeline_state, light.fragment_shader->type, light.fragment_shader->shader);

			device->setBindSet(pipeline_state, light_binding, light.ibl_environment->bindings);

			device->clearVertexStreams(pipeline_state);
			device->setVertexStream(pipeline_state, 0, light.mesh->vertex_buffer);

			device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, light.mesh->index_buffer, light.mesh->num_indices);
		}
	}
}

/*
 */
visual::IRenderPass *RenderPassPost::create(visual::RenderGraph *render_graph)
{
	RenderPassPost *result = new RenderPassPost();
	result->setRenderGraph(render_graph);

	return result;
}

/*
 */
void RenderPassPost::onInit()
{
	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, fullscreen_quad_vertex_shader->type, fullscreen_quad_vertex_shader->shader);
	device->setShader(pipeline_state, fragment_shader->type, fragment_shader->shader);

	device->clearVertexStreams(pipeline_state);
	device->setVertexStream(pipeline_state, 0, fullscreen_quad_mesh->vertex_buffer);
}

void RenderPassPost::onRender(foundation::render::CommandBuffer *command_buffer)
{
	device->drawIndexedPrimitiveInstanced(
		command_buffer,
		pipeline_state,
		fullscreen_quad_mesh->index_buffer,
		fullscreen_quad_mesh->num_indices
	);
}

/*
 */
visual::IRenderPass *RenderPassImGui::create(visual::RenderGraph *render_graph)
{
	RenderPassImGui *result = new RenderPassImGui();
	result->setRenderGraph(render_graph);

	return result;
}

/*
 */
void RenderPassImGui::onInit()
{
	assert(context);

	uint32_t width, height;
	unsigned char *pixels = nullptr;

	ImGuiIO &io = context->IO;
	io.Fonts->GetTexDataAsRGBA32(&pixels, reinterpret_cast<int *>(&width), reinterpret_cast<int *>(&height));

	font_texture = device->createTexture2D(width, height, 1, foundation::render::Format::R8G8B8A8_UNORM, pixels);
	font_bind_set = device->createBindSet();
	device->bindTexture(font_bind_set, 0, font_texture);

	io.Fonts->TexID = reinterpret_cast<ImTextureID>(font_bind_set);
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendRendererName = "Scapes";
}

void RenderPassImGui::onShutdown()
{
	device->destroyVertexBuffer(vertices);
	vertices = nullptr;

	device->destroyIndexBuffer(indices);
	indices = nullptr;

	device->destroyBindSet(font_bind_set);
	font_bind_set = nullptr;

	device->destroyTexture(font_texture);
	font_texture = nullptr;

	invalidateTextureIDs();
}

void RenderPassImGui::onRender(foundation::render::CommandBuffer *command_buffer)
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

				foundation::render::BindSet *bind_set = reinterpret_cast<foundation::render::BindSet *>(command.TextureId);
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

/*
 */
ImTextureID RenderPassImGui::fetchTextureID(const foundation::render::Texture *texture)
{
	auto it = registered_textures.find(texture);
	if (it != registered_textures.end())
		return it->second;

	foundation::render::BindSet *bind_set = device->createBindSet();
	device->bindTexture(bind_set, 0, texture);

	registered_textures.insert(std::make_pair(texture, bind_set));

	return reinterpret_cast<ImTextureID>(bind_set);
}

void RenderPassImGui::invalidateTextureIDs()
{
	for (auto &it : registered_textures)
		device->destroyBindSet(it.second);

	registered_textures.clear();
}

/*
 */
void RenderPassImGui::updateBuffers(const ImDrawData *draw_data)
{
	uint32_t num_indices = draw_data->TotalIdxCount;
	uint32_t num_vertices = draw_data->TotalVtxCount;

	if (num_vertices == 0 || num_indices == 0)
		return;

	constexpr size_t index_size = sizeof(ImDrawIdx);
	constexpr uint16_t vertex_size = static_cast<uint16_t>(sizeof(ImDrawVert));

	static_assert(index_size == 2 || index_size == 4, "Wrong ImDrawIdx size");
	static_assert(vertex_size == 20, "Wrong ImDrawVert size");

	foundation::render::IndexFormat index_format = foundation::render::IndexFormat::UINT16;
	if (index_size == 4)
		index_format = foundation::render::IndexFormat::UINT32;

	static const uint8_t num_attributes = 3;
	static foundation::render::VertexAttribute attributes[3] =
	{
		{ foundation::render::Format::R32G32_SFLOAT, offsetof(ImDrawVert, pos), },
		{ foundation::render::Format::R32G32_SFLOAT, offsetof(ImDrawVert, uv), },
		{ foundation::render::Format::R8G8B8A8_UNORM, offsetof(ImDrawVert, col), },
	};

	// resize index buffer
	if (index_buffer_size < index_size * num_indices)
	{
		index_buffer_size = index_size * num_indices;

		device->destroyIndexBuffer(indices);
		indices = device->createIndexBuffer(foundation::render::BufferType::DYNAMIC, index_format, num_indices, nullptr);
	}

	// resize vertex buffer
	if (vertex_buffer_size < vertex_size * num_vertices)
	{
		vertex_buffer_size = vertex_size * num_vertices;

		device->destroyVertexBuffer(vertices);
		vertices = device->createVertexBuffer(foundation::render::BufferType::DYNAMIC, vertex_size, num_vertices, num_attributes, attributes, nullptr);
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

void RenderPassImGui::setupRenderState(const ImDrawData *draw_data)
{
	device->setVertexStream(pipeline_state, 0, vertices);

	device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, vertex_shader.get()->shader);
	device->setShader(pipeline_state, foundation::render::ShaderType::FRAGMENT, fragment_shader.get()->shader);

	float L = draw_data->DisplayPos.x;
	float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
	float T = draw_data->DisplayPos.y;
	float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

	if (!device->isFlipped())
		std::swap(T,B);

	foundation::math::mat4 projection = foundation::math::ortho(L, R, B, T, 0.0f, 1.0f);

	device->setPushConstants(pipeline_state, sizeof(foundation::math::mat4), &projection);

	device->setBlending(pipeline_state, true);
	device->setBlendFactors(pipeline_state, foundation::render::BlendFactor::SRC_ALPHA, foundation::render::BlendFactor::ONE_MINUS_SRC_ALPHA);
	device->setCullMode(pipeline_state, foundation::render::CullMode::NONE);
	device->setDepthWrite(pipeline_state, false);
	device->setDepthTest(pipeline_state, false);
}
