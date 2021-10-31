#pragma once

#include <scapes/foundation/Fwd.h>

namespace scapes::visual
{
	class API;

	namespace components
	{
		struct Transform; // TODO: move to foundation components
		struct SkyLight;
		struct Renderable;
	}

	namespace resources
	{
		struct IBLTexture;
		struct Mesh;
		struct RenderMaterial;
		struct Shader;
		struct Texture;
	}

	typedef foundation::resources::ResourceHandle<resources::IBLTexture> IBLTextureHandle;
	typedef foundation::resources::ResourceHandle<resources::Mesh> MeshHandle;
	typedef foundation::resources::ResourceHandle<resources::RenderMaterial> RenderMaterialHandle;
	typedef foundation::resources::ResourceHandle<resources::Shader> ShaderHandle;
	typedef foundation::resources::ResourceHandle<resources::Texture> TextureHandle;
}
