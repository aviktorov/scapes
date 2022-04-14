#include <impl/RenderGraph.h>

namespace scapes::visual
{
	RenderGraph *RenderGraph::create(
		foundation::resources::ResourceManager *resource_manager,
		scapes::visual::hardware::Device *device,
		visual::shaders::Compiler *compiler,
		foundation::game::World *world,
		MeshHandle unit_quad
	)
	{
		return new impl::RenderGraph(resource_manager, device, compiler, world, unit_quad);
	}

	void RenderGraph::destroy(RenderGraph *RenderGraph)
	{
		delete RenderGraph;
	}
}
