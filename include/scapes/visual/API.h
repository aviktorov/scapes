#pragma once

#include <scapes/visual/Fwd.h>
#include <scapes/visual/Resources.h>

namespace scapes::visual
{
	class API
	{
	public:
		static SCAPES_API API *create(
			foundation::resources::ResourceManager *resource_manager,
			foundation::game::World *world,
			foundation::render::Device *device,
			foundation::shaders::Compiler *compiler
		);
		static SCAPES_API void destroy(API *api);

		virtual ~API() { }

	public:
		virtual foundation::resources::ResourceManager *getResourceManager() const = 0;
		virtual foundation::game::World *getWorld() const = 0;
		virtual foundation::render::Device *getDevice() const = 0;
		virtual foundation::shaders::Compiler *getCompiler() const = 0;

		virtual MeshHandle getUnitQuad() const = 0;
		virtual MeshHandle getUnitCube() const = 0;

		virtual TextureHandle getWhiteTexture() const = 0;
		virtual TextureHandle getGreyTexture() const = 0;
		virtual TextureHandle getBlackTexture() const = 0;
		virtual TextureHandle getNormalTexture() const = 0;
	};
}
