#include "RenderPasses.h"

#include <scapes/foundation/game/World.h>
#include <scapes/foundation/game/Query.h>

#include <scapes/visual/components/Components.h>
#include <scapes/visual/Shader.h>

#include <imgui.h>
#include <imgui_internal.h>

using namespace scapes;

namespace yaml = scapes::foundation::serde::yaml;

namespace c4
{
	/*
	 */
	SCAPES_INLINE bool from_chars(const yaml::csubstr buf, std::string *s)
	{
		*s = std::string(buf.data(), buf.size());
		return true;
	}
}

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
	graphics_pipeline = device->createGraphicsPipeline();

	onInit();
}

void RenderPassGraphicsBase::shutdown()
{
	onShutdown();

	clear();
}

void RenderPassGraphicsBase::render(visual::hardware::CommandBuffer command_buffer)
{
	if (!canRender())
		return;

	device->clearShaders(graphics_pipeline);

	if (vertex_shader.get())
		device->setShader(graphics_pipeline, vertex_shader->type, vertex_shader->shader);

	if (tessellation_control_shader.get())
		device->setShader(graphics_pipeline, tessellation_control_shader->type, tessellation_control_shader->shader);
	
	if (tessellation_evaluation_shader.get())
		device->setShader(graphics_pipeline, tessellation_evaluation_shader->type, tessellation_evaluation_shader->shader);

	if (geometry_shader.get())
		device->setShader(graphics_pipeline, geometry_shader->type, geometry_shader->shader);
	
	if (fragment_shader.get())
		device->setShader(graphics_pipeline, fragment_shader->type, fragment_shader->shader);

	device->clearBindSets(graphics_pipeline);

	uint8_t current_binding = 0;
	for (size_t i = 0; i < input_groups.size(); ++i)
	{
		const char *group_name = input_groups[i].c_str();
		visual::hardware::BindSet bindings = render_graph->getGroupBindings(group_name);
		device->setBindSet(graphics_pipeline, current_binding++, bindings);
	}

	for (size_t i = 0; i < input_render_buffers.size(); ++i)
	{
		const char *texture_name = input_render_buffers[i].c_str();
		visual::hardware::BindSet bindings = render_graph->getRenderBufferBindings(texture_name);
		device->setBindSet(graphics_pipeline, current_binding++, bindings);
	}

	if (render_pass_swapchain)
	{
		visual::hardware::SwapChain swap_chain = render_graph->getSwapChain();
		assert(swap_chain != SCAPES_NULL_HANDLE);

		device->beginRenderPass(command_buffer, render_pass_swapchain, swap_chain);
		onRender(command_buffer);
		device->endRenderPass(command_buffer);
	}

	if (render_pass_offscreen)
	{
		const char *render_buffers[32];
		uint32_t num_render_buffers = 0;

		for (size_t i = 0; i < color_outputs.size(); ++i)
			render_buffers[num_render_buffers++] = color_outputs[i].renderbuffer_name.c_str();

		if (has_depthstencil_output)
			render_buffers[num_render_buffers++] = depthstencil_output.renderbuffer_name.c_str();

		visual::hardware::FrameBuffer frame_buffer = render_graph->fetchFrameBuffer(num_render_buffers, render_buffers);
		assert(frame_buffer != SCAPES_NULL_HANDLE);

		device->beginRenderPass(command_buffer, render_pass_offscreen, frame_buffer);
		onRender(command_buffer);
		device->endRenderPass(command_buffer);
	}
}

void RenderPassGraphicsBase::invalidate()
{
	uint32_t width = render_graph->getWidth();
	uint32_t height = render_graph->getHeight();

	device->setViewport(graphics_pipeline, 0, 0, width, height);
	device->setScissor(graphics_pipeline, 0, 0, width, height);

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
bool RenderPassGraphicsBase::deserialize(const yaml::NodeRef node)
{

	for (const yaml::NodeRef child : node.children())
	{
		yaml::csubstr child_key = child.key();

		if (child_key.compare("input_groups") == 0)
		{
			for (const yaml::NodeRef input_group : child.children())
			{
				yaml::csubstr value = input_group.val();
				std::string group_name = std::string(value.data(), value.size());

				addInputGroup(group_name.c_str());
			}
		}

		else if (child_key.compare("input_renderbuffers") == 0)
		{
			for (const yaml::NodeRef input_renderbuffer : child.children())
			{
				yaml::csubstr value = input_renderbuffer.val();
				std::string renderbuffer_name = std::string(value.data(), value.size());

				addInputRenderBuffer(renderbuffer_name.c_str());
			}
		}

		else if (child_key.compare("output_colors") == 0)
		{
			for (const yaml::NodeRef output_color : child.children())
				deserializeFrameBufferOutput(output_color, false);
		}

		else if (child_key.compare("output_depthstencil") == 0)
			deserializeFrameBufferOutput(child, true);

		else if (child_key.compare("output_swapchain") == 0)
			deserializeSwapChainOutput(child);

		else if (child_key.compare("vertex_shader") == 0)
			deserializeShader(child, vertex_shader, visual::hardware::ShaderType::VERTEX);

		else if (child_key.compare("tessellation_control_shader") == 0)
			deserializeShader(child, tessellation_control_shader, visual::hardware::ShaderType::TESSELLATION_CONTROL);

		else if (child_key.compare("tessellation_evaluation_shader") == 0)
			deserializeShader(child, tessellation_evaluation_shader, visual::hardware::ShaderType::TESSELLATION_EVALUATION);

		else if (child_key.compare("geometry_shader") == 0)
			deserializeShader(child, geometry_shader, visual::hardware::ShaderType::GEOMETRY);

		else if (child_key.compare("fragment_shader") == 0)
			deserializeShader(child, fragment_shader, visual::hardware::ShaderType::FRAGMENT);
	}

	bool result = onDeserialize(node);

	return result;
}

void RenderPassGraphicsBase::deserializeFrameBufferOutput(yaml::NodeRef node, bool is_depthstencil)
{
	std::string name;
	visual::hardware::RenderPassLoadOp load_op = visual::hardware::RenderPassLoadOp::LOAD;
	visual::hardware::RenderPassStoreOp store_op = visual::hardware::RenderPassStoreOp::STORE;
	visual::hardware::RenderPassClearColor clear_color = {0.0f, 0.0f, 0.0f, 0.0f};
	visual::hardware::RenderPassClearDepthStencil clear_depthstencil = {0.0f, 0};

	for (const yaml::NodeRef child : node.children())
	{
		yaml::csubstr child_key = child.key();
		if (child_key.compare("name") == 0 && child.has_val())
		{
			yaml::csubstr output_color_value = child.val();
			name = std::string(output_color_value.data(), output_color_value.size());
		}

		else if (child_key.compare("load_op") == 0 && child.has_val())
			child >> load_op;

		else if (child_key.compare("store_op") == 0 && child.has_val())
			child >> store_op;

		else if (child_key.compare("clear_color") == 0 && child.has_val())
			child >> clear_color;

		else if (child_key.compare("clear_depthstencil") == 0 && child.has_val())
			child >> clear_depthstencil;
	}

	if (name.empty())
		return;
	
	if (is_depthstencil)
		setDepthStencilOutput(name.c_str(), load_op, store_op, clear_depthstencil);
	else
		addColorOutput(name.c_str(), load_op, store_op, clear_color);
};

void RenderPassGraphicsBase::deserializeSwapChainOutput(yaml::NodeRef node)
{
	visual::hardware::RenderPassLoadOp load_op = visual::hardware::RenderPassLoadOp::LOAD;
	visual::hardware::RenderPassStoreOp store_op = visual::hardware::RenderPassStoreOp::STORE;
	visual::hardware::RenderPassClearColor clear_color = {0.0f, 0.0f, 0.0f, 0.0f};

	for (const yaml::NodeRef child : node.children())
	{
		yaml::csubstr child_key = child.key();
		if (child_key.compare("load_op") == 0 && child.has_val())
			child >> load_op;

		else if (child_key.compare("store_op") == 0 && child.has_val())
			child >> store_op;

		else if (child_key.compare("clear_color") == 0 && child.has_val())
			child >> clear_color;
	}

	setSwapChainOutput(load_op, store_op, clear_color);
};

void RenderPassGraphicsBase::deserializeShader(yaml::NodeRef node, visual::ShaderHandle &handle, visual::hardware::ShaderType shader_type)
{
	if (!node.has_val())
		return;

	yaml::csubstr node_value = node.val();
	std::string path = std::string(node_value.data(), node_value.size());

	handle = resource_manager->fetch<visual::Shader>(path.c_str(), shader_type, device, compiler);
};

/*
 */
bool RenderPassGraphicsBase::serialize(yaml::NodeRef node)
{
	if (input_groups.size() > 0)
	{
		yaml::NodeRef container = node.append_child();
		container.set_key("input_groups");
		container |= yaml::SEQ;

		for (const std::string &name : input_groups)
		{
			yaml::NodeRef child = container.append_child();
			child << name.c_str();
		}
	}

	if (input_render_buffers.size() > 0)
	{
		yaml::NodeRef container = node.append_child();
		container.set_key("input_renderbuffers");
		container |= yaml::SEQ;

		for (const std::string &name : input_render_buffers)
		{
			yaml::NodeRef child = container.append_child();
			child << name.c_str();
		}
	}

	if (color_outputs.size() > 0)
	{
		yaml::NodeRef container = node.append_child();
		container.set_key("output_colors");
		container |= yaml::SEQ;

		for (const FrameBufferOutput color_output : color_outputs)
		{
			yaml::NodeRef child = container.append_child();

			serializeFrameBufferOutput(child, color_output, false);
		}
	}

	if (has_depthstencil_output)
	{
		yaml::NodeRef child = node.append_child();
		child.set_key("output_depthstencil");

		serializeFrameBufferOutput(child, depthstencil_output, true);
	}

	if (has_swapchain_output)
	{
		yaml::NodeRef child = node.append_child();
		child.set_key("output_swapchain");

		serializeSwapChainOutput(child, swapchain_output);
	}

	serializeShader(node, "vertex_shader", vertex_shader);
	serializeShader(node, "tessellation_control_shader", tessellation_control_shader);
	serializeShader(node, "tessellation_evaluation_shader", tessellation_evaluation_shader);
	serializeShader(node, "geometry_shader", geometry_shader);
	serializeShader(node, "fragment_shader", fragment_shader);

	bool result = onSerialize(node);

	return result;
}

void RenderPassGraphicsBase::serializeFrameBufferOutput(yaml::NodeRef node, const FrameBufferOutput &data, bool is_depthstencil)
{
	node |= yaml::MAP;
	node["name"] << data.renderbuffer_name.c_str();

	if (data.load_op != visual::hardware::RenderPassLoadOp::LOAD)
		node["load_op"] << data.load_op;

	if (data.store_op != visual::hardware::RenderPassStoreOp::STORE)
		node["store_op"] << data.store_op;

	if (data.load_op == visual::hardware::RenderPassLoadOp::CLEAR)
	{
		if (is_depthstencil)
			node["clear_depthstencil"] << data.clear_value.as_depth_stencil;
		else
			node["clear_color"] << data.clear_value.as_color;
	}
};

void RenderPassGraphicsBase::serializeSwapChainOutput(yaml::NodeRef node, const SwapChainOutput &data)
{
	node |= yaml::MAP;
	node["load_op"] << data.load_op;

	if (data.store_op != visual::hardware::RenderPassStoreOp::STORE)
		node["store_op"] << data.store_op;

	if (data.load_op == visual::hardware::RenderPassLoadOp::CLEAR)
		node["clear_color"] << data.clear_value;
};

void RenderPassGraphicsBase::serializeShader(yaml::NodeRef node, const char *name, visual::ShaderHandle handle)
{
	if (!handle.get())
		return;

	const foundation::io::URI &uri = resource_manager->getUri(handle);
	if (uri.empty())
		return;

	node[yaml::to_csubstr(name)] << uri.c_str();
};

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
	visual::hardware::RenderPassLoadOp load_op,
	visual::hardware::RenderPassStoreOp store_op,
	visual::hardware::RenderPassClearColor clear_value
)
{
	FrameBufferOutput output = {};
	output.renderbuffer_name = std::string(name);
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
		if (it->renderbuffer_name == texture_name)
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
	visual::hardware::RenderPassLoadOp load_op,
	visual::hardware::RenderPassStoreOp store_op,
	visual::hardware::RenderPassClearDepthStencil clear_value
)
{
	depthstencil_output.renderbuffer_name = std::string(name);
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
	visual::hardware::RenderPassLoadOp load_op,
	visual::hardware::RenderPassStoreOp store_op,
	visual::hardware::RenderPassClearColor clear_value
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
void RenderPassGraphicsBase::setRenderGraph(visual::RenderGraph *graph)
{
	render_graph = graph;

	device = nullptr;
	world = nullptr;
	resource_manager = nullptr;
	compiler = nullptr;
	unit_quad = visual::MeshHandle();

	if (render_graph)
	{
		device = render_graph->getDevice();
		world = render_graph->getWorld();
		resource_manager = render_graph->getResourceManager();
		compiler = render_graph->getCompiler();
		unit_quad = render_graph->getUnitQuad();
	}
}

/*
 */
void RenderPassGraphicsBase::clear()
{
	device->destroyRenderPass(render_pass_offscreen);
	render_pass_offscreen = SCAPES_NULL_HANDLE;

	device->destroyRenderPass(render_pass_swapchain);
	render_pass_swapchain = SCAPES_NULL_HANDLE;

	device->destroyGraphicsPipeline(graphics_pipeline);
	graphics_pipeline = SCAPES_NULL_HANDLE;

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

	visual::hardware::RenderPassAttachment attachments[32];
	uint32_t color_references[32];
	uint32_t depthstencil_reference = 0;

	visual::hardware::RenderPassDescription description;

	description.resolve_attachments = nullptr;
	description.color_attachments = color_references;
	description.num_color_attachments = static_cast<uint32_t>(color_outputs.size());

	uint32_t num_attachments = 0;
	for (; num_attachments < color_outputs.size(); ++num_attachments)
	{
		visual::hardware::RenderPassAttachment &attachment = attachments[num_attachments];
		const FrameBufferOutput &output = color_outputs[num_attachments];
		const std::string &texture_name = output.renderbuffer_name;

		attachment.clear_value = output.clear_value;
		attachment.load_op = output.load_op;
		attachment.store_op = output.store_op;
		attachment.samples = visual::hardware::Multisample::COUNT_1;
		attachment.format = render_graph->getRenderBufferFormat(texture_name.c_str());

		color_references[num_attachments] = num_attachments;
	}

	if (has_depthstencil_output)
	{
		visual::hardware::RenderPassAttachment &attachment = attachments[num_attachments];
		const std::string &texture_name = depthstencil_output.renderbuffer_name;

		attachment.clear_value = depthstencil_output.clear_value;
		attachment.load_op = depthstencil_output.load_op;
		attachment.store_op = depthstencil_output.store_op;
		attachment.samples = visual::hardware::Multisample::COUNT_1;
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

	visual::hardware::SwapChain swap_chain = render_graph->getSwapChain();
	assert(swap_chain != SCAPES_NULL_HANDLE);

	render_pass_swapchain = device->createRenderPass(swap_chain, swapchain_output.load_op, swapchain_output.store_op, swapchain_output.clear_value);
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
	device->clearVertexStreams(graphics_pipeline);
	device->setVertexStream(graphics_pipeline, 0, unit_quad->vertex_buffer);
}

void RenderPassPrepareOld::onInvalidate()
{
	first_frame = true;
}

void RenderPassPrepareOld::onRender(visual::hardware::CommandBuffer command_buffer)
{
	device->drawIndexedPrimitiveInstanced(
		command_buffer,
		graphics_pipeline,
		unit_quad->index_buffer,
		unit_quad->num_indices
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
	device->setCullMode(graphics_pipeline, visual::hardware::CullMode::BACK);
	device->setDepthTest(graphics_pipeline, true);
	device->setDepthWrite(graphics_pipeline, true);
}

void RenderPassGeometry::onRender(visual::hardware::CommandBuffer command_buffer)
{
	auto query = foundation::game::Query<visual::components::Transform, visual::components::Renderable>(world);

	query.begin();

	while (query.next())
	{
		uint32_t num_items = query.getNumComponents();
		visual::components::Transform *transforms = query.getComponents<visual::components::Transform>(0);
		visual::components::Renderable *renderables = query.getComponents<visual::components::Renderable>(1);

		for (uint32_t i = 0; i < num_items; ++i)
		{
			const visual::components::Transform &transform = transforms[i];
			const visual::components::Renderable &renderable = renderables[i];
			const foundation::math::mat4 &node_transform = transform.transform;

			visual::hardware::BindSet material_bindings = renderable.material->getGroupBindings(material_group_name.c_str());

			device->clearVertexStreams(graphics_pipeline);
			device->setVertexStream(graphics_pipeline, 0, renderable.mesh->vertex_buffer);

			device->setBindSet(graphics_pipeline, material_binding, material_bindings);
			device->setPushConstants(graphics_pipeline, static_cast<uint8_t>(sizeof(foundation::math::mat4)), &node_transform);

			device->drawIndexedPrimitiveInstanced(command_buffer, graphics_pipeline, renderable.mesh->index_buffer, renderable.mesh->num_indices);
		}
	}
}

bool RenderPassGeometry::onDeserialize(const yaml::NodeRef node)
{
	for (const yaml::NodeRef child : node.children())
	{
		yaml::csubstr child_key = child.key();

		if (child_key.compare("input_material_binding") == 0 && child.has_val())
			child >> material_binding;

		else if (child_key.compare("input_material_group_name") == 0 && child.has_val())
			child >> material_group_name;
	}

	return true;
}

bool RenderPassGeometry::onSerialize(yaml::NodeRef node)
{
	node["input_material_binding"] << material_binding;

	return true;
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
void RenderPassLBuffer::onInit()
{
	device->clearVertexStreams(graphics_pipeline);
	device->setVertexStream(graphics_pipeline, 0, unit_quad->vertex_buffer);
}

void RenderPassLBuffer::onRender(visual::hardware::CommandBuffer command_buffer)
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

			device->setBindSet(graphics_pipeline, light_binding, light.ibl_environment->bindings);
			device->drawIndexedPrimitiveInstanced(
				command_buffer,
				graphics_pipeline,
				unit_quad->index_buffer,
				unit_quad->num_indices
			);
		}
	}
}

bool RenderPassLBuffer::onDeserialize(const yaml::NodeRef node)
{
	for (const yaml::NodeRef child : node.children())
	{
		yaml::csubstr child_key = child.key();

		if (child_key.compare("input_light_binding") == 0 && child.has_val())
			child >> light_binding;
	}

	return true;
}

bool RenderPassLBuffer::onSerialize(yaml::NodeRef node)
{
	node["input_light_binding"] << light_binding;

	return true;
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
	device->clearVertexStreams(graphics_pipeline);
	device->setVertexStream(graphics_pipeline, 0, unit_quad->vertex_buffer);
}

void RenderPassPost::onRender(visual::hardware::CommandBuffer command_buffer)
{
	device->drawIndexedPrimitiveInstanced(
		command_buffer,
		graphics_pipeline,
		unit_quad->index_buffer,
		unit_quad->num_indices
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

	font_texture = device->createTexture2D(width, height, 1, visual::hardware::Format::R8G8B8A8_UNORM, pixels);
	font_bind_set = device->createBindSet();
	device->bindTexture(font_bind_set, 0, font_texture);

	io.Fonts->TexID = reinterpret_cast<ImTextureID>(font_bind_set);
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendRendererName = "Scapes";
}

void RenderPassImGui::onShutdown()
{
	device->destroyVertexBuffer(vertices);
	vertices = SCAPES_NULL_HANDLE;

	device->destroyIndexBuffer(indices);
	indices = SCAPES_NULL_HANDLE;

	device->destroyBindSet(font_bind_set);
	font_bind_set = SCAPES_NULL_HANDLE;

	device->destroyTexture(font_texture);
	font_texture = SCAPES_NULL_HANDLE;

	invalidateTextureIDs();
}

void RenderPassImGui::onRender(visual::hardware::CommandBuffer command_buffer)
{
	assert(context);
	assert(context->Viewports.size() > 0);

	const ImDrawData &draw_data = context->Viewports[0]->DrawDataP;
	const ImVec2 &clip_offset = draw_data.DisplayPos;
	const ImVec2 &clip_scale = draw_data.FramebufferScale;

	ImVec2 fb_size(draw_data.DisplaySize.x * clip_scale.x, draw_data.DisplaySize.y * clip_scale.y); 

	updateBuffers(draw_data);
	setupRenderState(draw_data);

	uint32_t index_offset = 0;
	int32_t vertex_offset = 0;

	device->setViewport(graphics_pipeline, 0, 0, static_cast<uint32_t>(fb_size.x), static_cast<uint32_t>(fb_size.y));

	for (int i = 0; i < draw_data.CmdListsCount; ++i)
	{
		const ImDrawList *list = draw_data.CmdLists[i];
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

				visual::hardware::BindSet bind_set = reinterpret_cast<visual::hardware::BindSet>(command.TextureId);
				device->setBindSet(graphics_pipeline, 0, bind_set);

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

				device->setScissor(graphics_pipeline, static_cast<int32_t>(x0), static_cast<int32_t>(y0), width, height);
				device->drawIndexedPrimitiveInstanced(command_buffer, graphics_pipeline, indices, num_indices, base_index, base_vertex, 1, 0);
			}
		}

		index_offset += list->IdxBuffer.Size;
		vertex_offset += list->VtxBuffer.Size;
	}
}

/*
 */
ImTextureID RenderPassImGui::fetchTextureID(visual::hardware::Texture texture)
{
	auto it = registered_textures.find(texture);
	if (it != registered_textures.end())
		return it->second;

	visual::hardware::BindSet bind_set = device->createBindSet();
	device->bindTexture(bind_set, 0, texture);

	registered_textures.insert(std::make_pair(texture, bind_set));

	return reinterpret_cast<ImTextureID>(bind_set);
}

/*
 */
void RenderPassImGui::invalidateTextureIDs()
{
	for (auto &it : registered_textures)
		device->destroyBindSet(it.second);

	registered_textures.clear();
}

/*
 */
void RenderPassImGui::updateBuffers(const ImDrawData &draw_data)
{
	uint32_t num_indices = draw_data.TotalIdxCount;
	uint32_t num_vertices = draw_data.TotalVtxCount;

	if (num_vertices == 0 || num_indices == 0)
		return;

	constexpr size_t index_size = sizeof(ImDrawIdx);
	constexpr uint16_t vertex_size = static_cast<uint16_t>(sizeof(ImDrawVert));

	static_assert(index_size == 2 || index_size == 4, "Wrong ImDrawIdx size");
	static_assert(vertex_size == 20, "Wrong ImDrawVert size");

	visual::hardware::IndexFormat index_format = visual::hardware::IndexFormat::UINT16;
	if (index_size == 4)
		index_format = visual::hardware::IndexFormat::UINT32;

	static const uint8_t num_attributes = 3;
	static visual::hardware::VertexAttribute attributes[3] =
	{
		{ visual::hardware::Format::R32G32_SFLOAT, offsetof(ImDrawVert, pos), },
		{ visual::hardware::Format::R32G32_SFLOAT, offsetof(ImDrawVert, uv), },
		{ visual::hardware::Format::R8G8B8A8_UNORM, offsetof(ImDrawVert, col), },
	};

	// resize index buffer
	if (index_buffer_size < index_size * num_indices)
	{
		index_buffer_size = index_size * num_indices;

		device->destroyIndexBuffer(indices);
		indices = device->createIndexBuffer(visual::hardware::BufferType::DYNAMIC, index_format, num_indices, nullptr);
	}

	// resize vertex buffer
	if (vertex_buffer_size < vertex_size * num_vertices)
	{
		vertex_buffer_size = vertex_size * num_vertices;

		device->destroyVertexBuffer(vertices);
		vertices = device->createVertexBuffer(visual::hardware::BufferType::DYNAMIC, vertex_size, num_vertices, num_attributes, attributes, nullptr);
	}

	ImDrawVert *vertex_data = reinterpret_cast<ImDrawVert *>(device->map(vertices));
	ImDrawIdx *index_data = reinterpret_cast<ImDrawIdx *>(device->map(indices));

	for (int i = 0; i < draw_data.CmdListsCount; ++i)
	{
		const ImDrawList *commands = draw_data.CmdLists[i];
		memcpy(vertex_data, commands->VtxBuffer.Data, commands->VtxBuffer.Size * vertex_size);
		memcpy(index_data, commands->IdxBuffer.Data, commands->IdxBuffer.Size * index_size);
		vertex_data += commands->VtxBuffer.Size;
		index_data += commands->IdxBuffer.Size;
	}

	device->unmap(vertices);
	device->unmap(indices);
}

void RenderPassImGui::setupRenderState(const ImDrawData &draw_data)
{
	device->clearVertexStreams(graphics_pipeline);
	device->setVertexStream(graphics_pipeline, 0, vertices);

	float L = draw_data.DisplayPos.x;
	float R = draw_data.DisplayPos.x + draw_data.DisplaySize.x;
	float T = draw_data.DisplayPos.y;
	float B = draw_data.DisplayPos.y + draw_data.DisplaySize.y;

	if (!device->isFlipped())
		std::swap(T,B);

	foundation::math::mat4 projection = foundation::math::ortho(L, R, B, T, 0.0f, 1.0f);

	device->clearPushConstants(graphics_pipeline);
	device->setPushConstants(graphics_pipeline, sizeof(foundation::math::mat4), &projection);

	device->setBlending(graphics_pipeline, true);
	device->setBlendFactors(graphics_pipeline, visual::hardware::BlendFactor::SRC_ALPHA, visual::hardware::BlendFactor::ONE_MINUS_SRC_ALPHA);
	device->setCullMode(graphics_pipeline, visual::hardware::CullMode::NONE);
	device->setDepthWrite(graphics_pipeline, false);
	device->setDepthTest(graphics_pipeline, false);
}

/*
 */
visual::IRenderPass *RenderPassSwapRenderBuffers::create(visual::RenderGraph *render_graph)
{
	RenderPassSwapRenderBuffers *result = new RenderPassSwapRenderBuffers();
	result->setRenderGraph(render_graph);

	return result;
}

/*
 */
RenderPassSwapRenderBuffers::RenderPassSwapRenderBuffers()
{

}

RenderPassSwapRenderBuffers::~RenderPassSwapRenderBuffers()
{
	clear();
}

/*
 */
void RenderPassSwapRenderBuffers::init()
{
}

void RenderPassSwapRenderBuffers::shutdown()
{
	clear();
}

void RenderPassSwapRenderBuffers::render(visual::hardware::CommandBuffer command_buffer)
{
	for (const SwapPair &pair : pairs)
		render_graph->swapRenderBuffers(pair.src.c_str(), pair.dst.c_str());
}

void RenderPassSwapRenderBuffers::invalidate()
{
}

/*
 */
bool RenderPassSwapRenderBuffers::deserialize(const yaml::NodeRef node)
{
	yaml::NodeRef child = node.find_child("pairs");
	if (child.empty())
		return true;

	if (!child.is_seq())
		return false;

	for (const yaml::NodeRef swap_pair : child.children())
	{
		std::string src;
		std::string dst;

		for (const yaml::NodeRef swap_pair_child : swap_pair.children())
		{
			yaml::csubstr swap_pair_key = swap_pair_child.key();

			if (swap_pair_key.compare("src") == 0 && swap_pair_child.has_val())
			{
				yaml::csubstr swap_pair_value = swap_pair_child.val();
				src = std::string(swap_pair_value.data(), swap_pair_value.size());
			}

			else if (swap_pair_key.compare("dst") == 0 && swap_pair_child.has_val())
			{
				yaml::csubstr swap_pair_value = swap_pair_child.val();
				dst = std::string(swap_pair_value.data(), swap_pair_value.size());
			}
		}

		if (src.empty() || dst.empty())
			continue;

		addSwapPair(src.c_str(), dst.c_str());
	}

	return true;
}

bool RenderPassSwapRenderBuffers::serialize(yaml::NodeRef node)
{
	if (pairs.size() > 0)
	{
		yaml::NodeRef container = node.append_child();
		container.set_key("pairs");
		container |= yaml::SEQ;

		for (const SwapPair &pair : pairs)
		{
			yaml::NodeRef child = container.append_child();
			child |= yaml::MAP;

			child["src"] << pair.src.c_str();
			child["dst"] << pair.dst.c_str();
		}
	}

	return true;
}

/*
 */
void RenderPassSwapRenderBuffers::addSwapPair(const char *src, const char *dst)
{
	SwapPair pair;
	pair.src = std::string(src);
	pair.dst = std::string(dst);

	pairs.push_back(pair);
}

void RenderPassSwapRenderBuffers::removeAllSwapPairs()
{
	pairs.clear();
}

/*
 */
void RenderPassSwapRenderBuffers::clear()
{
	removeAllSwapPairs();
}
