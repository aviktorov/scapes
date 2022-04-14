#pragma once

#include <scapes/visual/Fwd.h>
#include <scapes/visual/Resources.h>

namespace scapes::visual
{
	/*
	 */
	class GlbImporter
	{
	public:
		static SCAPES_API GlbImporter *create(
			foundation::resources::ResourceManager *resource_manager,
			foundation::game::World *world,
			foundation::render::Device *device
		);
		static SCAPES_API void destroy(GlbImporter *importer);
		
		virtual ~GlbImporter() { }

	public:
		virtual bool import(const foundation::io::URI &uri, RenderMaterialHandle default_material) = 0;
	};
}
