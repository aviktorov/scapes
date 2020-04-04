#pragma once

#include <volk.h>
#include <string>

#include "VulkanResourceManager.h"

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
class RenderScene
{
public:
	RenderScene(render::backend::Driver *driver)
		: resources(driver) { }

	void init();
	void shutdown();

	inline const VulkanShader *getPBRVertexShader() const { return resources.getShader(config::Shaders::PBRVertex); }
	inline const VulkanShader *getPBRFragmentShader() const { return resources.getShader(config::Shaders::PBRFragment); }
	inline const VulkanShader *getSkyboxVertexShader() const { return resources.getShader(config::Shaders::SkyboxVertex); }
	inline const VulkanShader *getSkyboxFragmentShader() const { return resources.getShader(config::Shaders::SkyboxFragment); }

	inline const VulkanShader *getCubeVertexShader() const { return resources.getShader(config::Shaders::CubeVertex); }
	inline const VulkanShader *getHDRIToFragmentShader() const { return resources.getShader(config::Shaders::HDRIToCubeFragment); }
	inline const VulkanShader *getCubeToPrefilteredSpecularShader() const { return resources.getShader(config::Shaders::CubeToPrefilteredSpecular); }
	inline const VulkanShader *getDiffuseIrradianceFragmentShader() const { return resources.getShader(config::Shaders::DiffuseIrradianceFragment); }

	inline const VulkanShader *getBakedBRDFVertexShader() const { return resources.getShader(config::Shaders::BakedBRDFVertex); }
	inline const VulkanShader *getBakedBRDFFragmentShader() const { return resources.getShader(config::Shaders::BakedBRDFFragment); }

	inline const VulkanTexture *getAlbedoTexture() const { return resources.getTexture(config::Textures::Albedo); }
	inline const VulkanTexture *getNormalTexture() const { return resources.getTexture(config::Textures::Normal); }
	inline const VulkanTexture *getAOTexture() const { return resources.getTexture(config::Textures::AO); }
	inline const VulkanTexture *getShadingTexture() const { return resources.getTexture(config::Textures::Shading); }
	inline const VulkanTexture *getEmissionTexture() const { return resources.getTexture(config::Textures::Emission); }
	inline const VulkanTexture *getHDRTexture(int index) const { return resources.getTexture(config::Textures::EnvironmentBase + index); }
	const char *getHDRTexturePath(int index) const;
	size_t getNumHDRTextures() const;

	inline const VulkanMesh *getMesh() const { return resources.getMesh(config::Meshes::Helmet); }
	inline const VulkanMesh *getSkybox() const { return resources.getMesh(config::Meshes::Skybox); }

	void reloadShaders();

private:
	VulkanResourceManager resources;
};
