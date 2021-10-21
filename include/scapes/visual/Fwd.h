#pragma once

#include <common/ResourceManager.h>

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

	typedef ::ResourceHandle<resources::IBLTexture> IBLTextureHandle;
	typedef ::ResourceHandle<resources::Mesh> MeshHandle;
	typedef ::ResourceHandle<resources::RenderMaterial> RenderMaterialHandle;
	typedef ::ResourceHandle<resources::Shader> ShaderHandle;
	typedef ::ResourceHandle<resources::Texture> TextureHandle;
}
