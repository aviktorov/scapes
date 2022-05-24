#pragma once

#include <scapes/visual/GlbImporter.h>

struct cgltf_mesh;

namespace scapes::visual::impl
{
	/*
	 */
	class GlbImporter : public visual::GlbImporter
	{
	public:
		GlbImporter(
			foundation::resources::ResourceManager *resource_manager,
			foundation::game::World *world,
			hardware::Device *device
		);
		~GlbImporter() final;

		bool import(const foundation::io::URI &uri, MaterialHandle default_material) final;

	private:
		MeshHandle import_mesh(const cgltf_mesh *mesh);

	private:
		foundation::resources::ResourceManager *resource_manager {nullptr};
		foundation::game::World *world {nullptr};
		hardware::Device *device {nullptr};
	};
}
