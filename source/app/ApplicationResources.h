#pragma once

#include "ResourceManager.h"

namespace config
{
	enum Meshes
	{
		Helmet = 0,
		Skybox,
	};

	enum Shaders
	{
		CubemapVertex = 0,
		FullscreenQuadVertex,
		EquirectangularProjectionFragment,
		PrefilteredSpecularCubemapFragment,
		DiffuseIrradianceCubemapFragment,
		BakedBRDFFragment,
		SkylightDeferredFragment,
		GBufferVertex,
		GBufferFragment,
		SSAOFragment,
		SSAOBlurFragment,
		SSRTraceFragment,
		SSRResolveFragment,
		CompositeFragment,
		TemporalFilterFragment,
		TonemappingFragment,
		FinalFragment,
	};

	enum Textures
	{
		Albedo = 0,
		Normal,
		AO,
		Shading,
		Emission,
		EnvironmentBase,
	};
}

/*
 */
class ApplicationResources
{
public:
	ApplicationResources(render::backend::Driver *driver, render::shaders::Compiler *compiler)
		: driver(driver), compiler(compiler), resources(driver, compiler) { }

	virtual ~ApplicationResources();

	void init();
	void shutdown();

	inline const render::Shader *getShader(config::Shaders index) const { return resources.getShader(index); }

	inline const render::Texture *getAlbedoTexture() const { return resources.getTexture(config::Textures::Albedo); }
	inline const render::Texture *getNormalTexture() const { return resources.getTexture(config::Textures::Normal); }
	inline const render::Texture *getAOTexture() const { return resources.getTexture(config::Textures::AO); }
	inline const render::Texture *getShadingTexture() const { return resources.getTexture(config::Textures::Shading); }
	inline const render::Texture *getEmissionTexture() const { return resources.getTexture(config::Textures::Emission); }

	inline const render::Texture *getBlueNoiseTexture() const { return blue_noise; }

	inline const render::Texture *getHDRTexture(int index) const { return resources.getTexture(config::Textures::EnvironmentBase + index); }
	inline const render::Texture *getHDREnvironmentCubemap(int index) const { return environment_cubemaps[index]; }
	inline const render::Texture *getHDRIrradianceCubemap(int index) const { return irradiance_cubemaps[index]; }
	const char *getHDRTexturePath(int index) const;
	size_t getNumHDRTextures() const;

	inline const render::Texture *getBakedBRDFTexture() const { return baked_brdf; }

	inline const render::Mesh *getMesh() const { return resources.getMesh(config::Meshes::Helmet); }
	inline const render::Mesh *getSkybox() const { return resources.getMesh(config::Meshes::Skybox); }

	void reloadShaders();

private:
	render::backend::Driver *driver {nullptr};
	render::shaders::Compiler *compiler {nullptr};
	ResourceManager resources;

	render::Texture *baked_brdf {nullptr};
	std::vector<render::Texture *> environment_cubemaps;
	std::vector<render::Texture *> irradiance_cubemaps;

	render::Texture *blue_noise {nullptr};
};
