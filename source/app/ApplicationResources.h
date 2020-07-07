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
		PBRVertex = 0,
		PBRFragment,
		SkyboxVertex,
		SkyboxFragment,
		CubeVertex,
		HDRIToCubeFragment,
		CubeToPrefilteredSpecular,
		DiffuseIrradianceFragment,
		BakedBRDFVertex,
		BakedBRDFFragment,
		GBufferVertex,
		GBufferFragment,
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
	ApplicationResources(render::backend::Driver *driver)
		: resources(driver) { }

	virtual ~ApplicationResources();

	void init();
	void shutdown();

	inline const render::Shader *getPBRVertexShader() const { return resources.getShader(config::Shaders::PBRVertex); }
	inline const render::Shader *getPBRFragmentShader() const { return resources.getShader(config::Shaders::PBRFragment); }
	inline const render::Shader *getSkyboxVertexShader() const { return resources.getShader(config::Shaders::SkyboxVertex); }
	inline const render::Shader *getSkyboxFragmentShader() const { return resources.getShader(config::Shaders::SkyboxFragment); }

	inline const render::Shader *getCubeVertexShader() const { return resources.getShader(config::Shaders::CubeVertex); }
	inline const render::Shader *getHDRIToFragmentShader() const { return resources.getShader(config::Shaders::HDRIToCubeFragment); }
	inline const render::Shader *getCubeToPrefilteredSpecularShader() const { return resources.getShader(config::Shaders::CubeToPrefilteredSpecular); }
	inline const render::Shader *getDiffuseIrradianceFragmentShader() const { return resources.getShader(config::Shaders::DiffuseIrradianceFragment); }

	inline const render::Shader *getBakedBRDFVertexShader() const { return resources.getShader(config::Shaders::BakedBRDFVertex); }
	inline const render::Shader *getBakedBRDFFragmentShader() const { return resources.getShader(config::Shaders::BakedBRDFFragment); }

	inline const render::Shader *getGBufferVertexShader() const { return resources.getShader(config::Shaders::GBufferVertex); }
	inline const render::Shader *getGBufferFragmentShader() const { return resources.getShader(config::Shaders::GBufferFragment); }

	inline const render::Texture *getAlbedoTexture() const { return resources.getTexture(config::Textures::Albedo); }
	inline const render::Texture *getNormalTexture() const { return resources.getTexture(config::Textures::Normal); }
	inline const render::Texture *getAOTexture() const { return resources.getTexture(config::Textures::AO); }
	inline const render::Texture *getShadingTexture() const { return resources.getTexture(config::Textures::Shading); }
	inline const render::Texture *getEmissionTexture() const { return resources.getTexture(config::Textures::Emission); }
	inline const render::Texture *getHDRTexture(int index) const { return resources.getTexture(config::Textures::EnvironmentBase + index); }
	const char *getHDRTexturePath(int index) const;
	size_t getNumHDRTextures() const;

	inline const render::Mesh *getMesh() const { return resources.getMesh(config::Meshes::Helmet); }
	inline const render::Mesh *getSkybox() const { return resources.getMesh(config::Meshes::Skybox); }

	void reloadShaders();

private:
	ResourceManager resources;
};
