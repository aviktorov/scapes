#pragma once

#include <scapes/foundation/components/Components.h>

#include <scapes/visual/API.h>
#include <scapes/visual/Components.h>
#include <scapes/visual/Resources.h>

#include <vector>
#include <map>
#include <string>

namespace scapes::visual
{
	class APIImpl : public API
	{
	public:
		APIImpl(
			foundation::resources::ResourceManager *resource_manager,
			foundation::game::World *world,
			foundation::render::Device *device,
			foundation::shaders::Compiler *compiler
		);

		~APIImpl();

		SCAPES_INLINE foundation::resources::ResourceManager *getResourceManager() const final { return resource_manager; }
		SCAPES_INLINE foundation::game::World *getWorld() const final { return world; }
		SCAPES_INLINE foundation::render::Device *getDevice() const final { return device; }
		SCAPES_INLINE foundation::shaders::Compiler *getCompiler() const final { return compiler; }

		SCAPES_INLINE MeshHandle getUnitQuad() const final { return unit_quad; }
		SCAPES_INLINE MeshHandle getUnitCube() const final { return unit_cube; }

		SCAPES_INLINE TextureHandle getWhiteTexture() const final { return default_white; }
		SCAPES_INLINE TextureHandle getGreyTexture() const final { return default_grey; }
		SCAPES_INLINE TextureHandle getBlackTexture() const final { return default_black; }
		SCAPES_INLINE TextureHandle getNormalTexture() const final { return default_normal; }

	private:
		MeshHandle generateMeshQuad(float size);
		MeshHandle generateMeshCube(float size);

		TextureHandle generateTexture(uint8_t r, uint8_t g, uint8_t b);

	private:
		foundation::resources::ResourceManager *resource_manager {nullptr};
		foundation::game::World *world {nullptr};
		foundation::render::Device *device {nullptr};
		foundation::shaders::Compiler *compiler {nullptr};

		MeshHandle unit_quad;
		MeshHandle unit_cube;

		TextureHandle default_white;
		TextureHandle default_grey;
		TextureHandle default_black;
		TextureHandle default_normal;
	};
}
