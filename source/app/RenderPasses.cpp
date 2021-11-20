#include "RenderPasses.h"

#include <scapes/foundation/game/World.h>
#include <scapes/foundation/game/Query.h>

#include <scapes/foundation/components/Components.h>
#include <scapes/visual/Components.h>

using namespace scapes;

/*
 */
RenderPassGraphicsBase::~RenderPassGraphicsBase()
{
	clear();

	removeAllInputParameterGroups();
	removeAllInputTextures();
	removeAllColorOutputs();
	removeDepthStencilOutput();
}

/*
 */
void RenderPassGraphicsBase::init(const visual::RenderGraph *graph)
{
	render_graph = graph;
	device = render_graph->getDevice();
	world = render_graph->getWorld();

	createRenderPass();
	pipeline_state = device->createPipelineState();

	invalidate();
}

void RenderPassGraphicsBase::shutdown()
{
	clear();
}

void RenderPassGraphicsBase::invalidate()
{
	uint32_t width = render_graph->getWidth();
	uint32_t height = render_graph->getHeight();

	device->setViewport(pipeline_state, 0, 0, width, height);
	device->setScissor(pipeline_state, 0, 0, width, height);

	device->destroyBindSet(texture_bindings);
	texture_bindings = nullptr;

	device->destroyFrameBuffer(frame_buffer);
	frame_buffer = nullptr;

	createFrameBuffer();

	device->clearBindSets(pipeline_state);
	uint8_t current_index = 0;
	for (current_index; current_index < input_groups.size(); ++current_index)
	{
		const char *group_name = input_groups[current_index].c_str();
		foundation::render::BindSet *bindings = render_graph->getParameterGroupBindings(group_name);
		device->setBindSet(pipeline_state, current_index, bindings);
	}

	if (input_textures.size() == 0)
		return;

	texture_bindings = device->createBindSet();
	for (uint8_t i = 0; i < input_textures.size(); ++i)
	{
		const char *texture_name = input_textures[i].c_str();
		foundation::render::Texture *texture = render_graph->getTexture(texture_name);
		device->bindTexture(texture_bindings, i, texture);
	}

	device->setBindSet(pipeline_state, current_index, texture_bindings);
}

/*
 */
void RenderPassGraphicsBase::addInputParameterGroup(const char *name)
{
	input_groups.push_back(std::string(name));
}

void RenderPassGraphicsBase::removeInputParameterGroup(const char *name)
{
	const std::string group_name = std::string(name);
	auto it = std::find(input_groups.begin(), input_groups.end(), group_name);
	if (it == input_groups.end())
		return;

	input_groups.erase(it);
}

void RenderPassGraphicsBase::removeAllInputParameterGroups()
{
	input_groups.clear();
}

/*
 */
void RenderPassGraphicsBase::addInputTexture(const char *name)
{
	input_textures.push_back(std::string(name));
}

void RenderPassGraphicsBase::removeInputTexture(const char *name)
{
	const std::string texture_name = std::string(name);
	auto it = std::find(input_textures.begin(), input_textures.end(), texture_name);
	if (it == input_textures.end())
		return;

	input_textures.erase(it);
}

void RenderPassGraphicsBase::removeAllInputTextures()
{
	input_textures.clear();
}

/*
 */
void RenderPassGraphicsBase::addColorOutput(
	const char *name,
	foundation::render::RenderPassLoadOp load_op,
	foundation::render::RenderPassStoreOp store_op,
	foundation::render::RenderPassClearValue clear_value
)
{
	Output output = {};
	output.texture_name = std::string(name);
	output.load_op = load_op;
	output.store_op = store_op;
	output.clear_value = clear_value;

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
	foundation::render::RenderPassClearValue clear_value
)
{
	depthstencil_output.texture_name = std::string(name);
	depthstencil_output.load_op = load_op;
	depthstencil_output.store_op = store_op;
	depthstencil_output.clear_value = clear_value;

	has_depthstencil_output = true;
}

void RenderPassGraphicsBase::removeDepthStencilOutput()
{
	has_depthstencil_output = false;
}

/*
 */
void RenderPassGraphicsBase::clear()
{
	device->destroyRenderPass(render_pass);
	render_pass = nullptr;

	device->destroyFrameBuffer(frame_buffer);
	frame_buffer = nullptr;

	device->destroyBindSet(texture_bindings);
	texture_bindings = nullptr;

	device->destroyPipelineState(pipeline_state);
	pipeline_state = nullptr;
}

/*
 */
void RenderPassGraphicsBase::createRenderPass()
{
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
		const Output &output = color_outputs[num_attachments];
		const std::string &texture_name = output.texture_name;

		attachment.clear_value = output.clear_value;
		attachment.load_op = output.load_op;
		attachment.store_op = output.store_op;
		attachment.samples = foundation::render::Multisample::COUNT_1;
		attachment.format = render_graph->getTextureRenderBufferFormat(texture_name.c_str());

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
		attachment.format = render_graph->getTextureRenderBufferFormat(texture_name.c_str());

		depthstencil_reference = num_attachments;
		description.depthstencil_attachment = &depthstencil_reference;

		++num_attachments;
	}

	render_pass = device->createRenderPass(num_attachments, attachments, description);
}

/*
 */
void RenderPassGraphicsBase::createFrameBuffer()
{
	foundation::render::FrameBufferAttachment attachments[32];
	uint32_t num_attachments = 0;

	for (; num_attachments < color_outputs.size(); ++num_attachments)
	{
		foundation::render::FrameBufferAttachment &attachment = attachments[num_attachments];
		const Output &output = color_outputs[num_attachments];
		const std::string &texture_name = output.texture_name;

		attachment.texture = render_graph->getTextureRenderBuffer(texture_name.c_str());
		attachment.base_layer = 0;
		attachment.base_mip = 0;
		attachment.num_layers = 1;
	}

	if (has_depthstencil_output)
	{
		foundation::render::FrameBufferAttachment &attachment = attachments[num_attachments];
		const std::string &texture_name = depthstencil_output.texture_name;

		attachment.texture = render_graph->getTextureRenderBuffer(texture_name.c_str());
		attachment.base_layer = 0;
		attachment.base_mip = 0;
		attachment.num_layers = 1;

		++num_attachments;
	}

	frame_buffer = device->createFrameBuffer(num_attachments, attachments);
}

/*
 */
void RenderPassGeometry::init(const visual::RenderGraph *render_graph)
{
	RenderPassGraphicsBase::init(render_graph);

	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, vertex_shader->type, vertex_shader->shader);
	device->setShader(pipeline_state, fragment_shader->type, fragment_shader->shader);

	device->setCullMode(pipeline_state, foundation::render::CullMode::BACK);
	device->setDepthTest(pipeline_state, true);
	device->setDepthWrite(pipeline_state, true);
}

void RenderPassGeometry::render(foundation::render::CommandBuffer *command_buffer)
{
	device->beginRenderPass(command_buffer, render_pass, frame_buffer);

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

	device->endRenderPass(command_buffer);
}

/*
 */
void RenderPassSkylight::init(const visual::RenderGraph *render_graph)
{
	RenderPassGraphicsBase::init(render_graph);
}

void RenderPassSkylight::render(foundation::render::CommandBuffer *command_buffer)
{
	device->beginRenderPass(command_buffer, render_pass, frame_buffer);

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

	device->endRenderPass(command_buffer);
}

/*
 */
void RenderPassPost::init(const visual::RenderGraph *render_graph)
{
	RenderPassGraphicsBase::init(render_graph);

	device->clearShaders(pipeline_state);
	device->setShader(pipeline_state, fullscreen_quad_vertex_shader->type, fullscreen_quad_vertex_shader->shader);
	device->setShader(pipeline_state, fragment_shader->type, fragment_shader->shader);

	device->clearVertexStreams(pipeline_state);
	device->setVertexStream(pipeline_state, 0, fullscreen_quad_mesh->vertex_buffer);

}

void RenderPassPost::render(foundation::render::CommandBuffer *command_buffer)
{
	device->beginRenderPass(command_buffer, render_pass, frame_buffer);

	device->drawIndexedPrimitiveInstanced(
		command_buffer,
		pipeline_state,
		fullscreen_quad_mesh->index_buffer,
		fullscreen_quad_mesh->num_indices
	);

	device->endRenderPass(command_buffer);
}
