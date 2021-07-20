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

class Mesh;
class Shader;
class Texture;

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

	inline const Shader *getShader(config::Shaders index) const { return resources.getShader(index); }

	inline const Texture *getAlbedoTexture() const { return resources.getTexture(config::Textures::Albedo); }
	inline const Texture *getNormalTexture() const { return resources.getTexture(config::Textures::Normal); }
	inline const Texture *getAOTexture() const { return resources.getTexture(config::Textures::AO); }
	inline const Texture *getShadingTexture() const { return resources.getTexture(config::Textures::Shading); }
	inline const Texture *getEmissionTexture() const { return resources.getTexture(config::Textures::Emission); }

	inline const Texture *getBlueNoiseTexture() const { return blue_noise; }

	inline const Texture *getHDRTexture(int index) const { return resources.getTexture(config::Textures::EnvironmentBase + index); }
	inline const Texture *getHDREnvironmentCubemap(int index) const { return environment_cubemaps[index]; }
	inline const Texture *getHDRIrradianceCubemap(int index) const { return irradiance_cubemaps[index]; }
	const char *getHDRTexturePath(int index) const;
	size_t getNumHDRTextures() const;

	inline const Texture *getBakedBRDFTexture() const { return baked_brdf; }

	inline const Mesh *getMesh() const { return resources.getMesh(config::Meshes::Helmet); }
	inline const Mesh *getSkybox() const { return resources.getMesh(config::Meshes::Skybox); }
	inline const Mesh *getFullscreenQuad() const { return fullscreen_quad; }

	void reloadShaders();

private:
	render::backend::Driver *driver {nullptr};
	render::shaders::Compiler *compiler {nullptr};
	ResourceManager resources;

	Mesh *fullscreen_quad {nullptr};

	Texture *baked_brdf {nullptr};
	std::vector<Texture *> environment_cubemaps;
	std::vector<Texture *> irradiance_cubemaps;

	Texture *blue_noise {nullptr};
};
