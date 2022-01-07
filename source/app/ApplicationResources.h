#pragma once

#include <vector>

#include <scapes/visual/API.h>

namespace config
{
	enum Shaders
	{
		DefaultVertex = 0,
		DefaultCubemapGeometry,
		EquirectangularProjectionFragment,
		PrefilteredSpecularCubemapFragment,
		DiffuseIrradianceCubemapFragment,
		BakedBRDFFragment,
	};
}

/*
 */
class ApplicationResources
{
public:
	ApplicationResources(scapes::visual::API *visual_api)
		: visual_api(visual_api) { }

	virtual ~ApplicationResources();

	void init();
	void shutdown();

	inline scapes::visual::IBLTextureHandle getIBLTexture(int index) const { return loaded_ibl_textures[index]; }
	inline size_t getNumIBLTextures() const { return loaded_ibl_textures.size(); }
	const char *getIBLTexturePath(int index) const;

	inline scapes::visual::TextureHandle getBakedBRDFTexture() const { return baked_brdf; }

	inline scapes::visual::TextureHandle getDefaultAlbedo() const { return default_albedo; }
	inline scapes::visual::TextureHandle getDefaultNormal() const { return default_normal; }
	inline scapes::visual::TextureHandle getDefaultRoughness() const { return default_roughness; }
	inline scapes::visual::TextureHandle getDefaultMetalness() const { return default_metalness; }

	inline scapes::visual::ShaderHandle getShader(int index) const { return loaded_shaders[index]; }

private:
	scapes::visual::API *visual_api {nullptr};

	scapes::visual::TextureHandle baked_brdf;
	scapes::visual::TextureHandle blue_noise;

	scapes::visual::TextureHandle default_albedo;
	scapes::visual::TextureHandle default_normal;
	scapes::visual::TextureHandle default_roughness;
	scapes::visual::TextureHandle default_metalness;

	std::vector<scapes::visual::ShaderHandle > loaded_shaders;
	std::vector<scapes::visual::IBLTextureHandle > loaded_ibl_textures;
};
