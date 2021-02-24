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
		SkyLightVertex,
		SkyLightFragment,
		PassGBufferVertex,
		PassGBufferFragment,
		PassSSAOVertex,
		PassSSAOFragment,
		PassSSAOBlurVertex,
		PassSSAOBlurFragment,
		PassSSRTraceVertex,
		PassSSRTraceFragment,
		PassSSRResolveVertex,
		PassSSRResolveFragment,
		PassCompositeVertex,
		PassCompositeFragment,
		PassTemporalFilterVertex,
		PassTemporalFilterFragment,
		PassFinalVertex,
		PassFinalFragment,
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

	inline const render::Shader *getGBufferVertexShader() const { return resources.getShader(config::Shaders::PassGBufferVertex); }
	inline const render::Shader *getGBufferFragmentShader() const { return resources.getShader(config::Shaders::PassGBufferFragment); }

	inline const render::Shader *getSSAOVertexShader() const { return resources.getShader(config::Shaders::PassSSAOVertex); }
	inline const render::Shader *getSSAOFragmentShader() const { return resources.getShader(config::Shaders::PassSSAOFragment); }

	inline const render::Shader *getSSAOBlurVertexShader() const { return resources.getShader(config::Shaders::PassSSAOBlurVertex); }
	inline const render::Shader *getSSAOBlurFragmentShader() const { return resources.getShader(config::Shaders::PassSSAOBlurFragment); }

	inline const render::Shader *getSSRTraceVertexShader() const { return resources.getShader(config::Shaders::PassSSRTraceVertex); }
	inline const render::Shader *getSSRTraceFragmentShader() const { return resources.getShader(config::Shaders::PassSSRTraceFragment); }

	inline const render::Shader *getSSRResolveVertexShader() const { return resources.getShader(config::Shaders::PassSSRResolveVertex); }
	inline const render::Shader *getSSRResolveFragmentShader() const { return resources.getShader(config::Shaders::PassSSRResolveFragment); }

	inline const render::Shader *getCompositeVertexShader() const { return resources.getShader(config::Shaders::PassCompositeVertex); }
	inline const render::Shader *getCompositeFragmentShader() const { return resources.getShader(config::Shaders::PassCompositeFragment); }

	inline const render::Shader *getTemporalFilterVertexShader() const { return resources.getShader(config::Shaders::PassTemporalFilterVertex); }
	inline const render::Shader *getTemporalFilterFragmentShader() const { return resources.getShader(config::Shaders::PassTemporalFilterFragment); }

	inline const render::Shader *getFinalVertexShader() const { return resources.getShader(config::Shaders::PassFinalVertex); }
	inline const render::Shader *getFinalFragmentShader() const { return resources.getShader(config::Shaders::PassFinalFragment); }

	inline const render::Shader *getSkyLightVertexShader() const { return resources.getShader(config::Shaders::SkyLightVertex); }
	inline const render::Shader *getSkyLightFragmentShader() const { return resources.getShader(config::Shaders::SkyLightFragment); }

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
