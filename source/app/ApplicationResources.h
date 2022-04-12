#pragma once

#include <vector>

#include <scapes/visual/API.h>

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

private:
	scapes::visual::API *visual_api {nullptr};

	scapes::visual::TextureHandle baked_brdf;

	std::vector<scapes::visual::ShaderHandle> loaded_shaders;
	std::vector<scapes::visual::IBLTextureHandle> loaded_ibl_textures;
};
