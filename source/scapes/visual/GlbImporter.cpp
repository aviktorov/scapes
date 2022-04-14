#include <impl/GlbImporter.h>

namespace scapes::visual
{
	GlbImporter *GlbImporter::create(
		foundation::resources::ResourceManager *resource_manager,
		foundation::game::World *world,
		foundation::render::Device *device
	)
	{
		return new impl::GlbImporter(resource_manager, world, device);
	}

	void GlbImporter::destroy(GlbImporter *importer)
	{
		delete importer;
	}
}
