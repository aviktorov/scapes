#pragma once

#include <volk.h>
#include <string>

#include "VulkanRendererContext.h"
#include "VulkanMesh.h"
#include "VulkanTexture.h"
#include "VulkanShader.h"

/*
 */
class RenderScene
{
public:
	RenderScene(const VulkanRendererContext &context)
		: context(context),
		albedoTexture(context), normalTexture(context),	aoTexture(context),
		shadingTexture(context), emissionTexture(context), hdrTexture(context),
		pbrVertexShader(context), pbrFragmentShader(context),
		skyboxVertexShader(context), skyboxFragmentShader(context),
		mesh(context), skybox(context)
	{ }

	void init(
		const std::string &pbrVertexShaderFile,
		const std::string &pbrFragmentShaderFile,
		const std::string &skyboxVertexShaderFile,
		const std::string &skyboxFragmentShaderFile,
		const std::string &albedoFile,
		const std::string &normalFile,
		const std::string &aoFile,
		const std::string &shadingFile,
		const std::string &emissionFile,
		const std::string &hdrFile,
		const std::string &modelFile
	);
	void shutdown();

	inline const VulkanShader &getPBRVertexShader() const { return pbrVertexShader; }
	inline const VulkanShader &getPBRFragmentShader() const { return pbrFragmentShader; }
	inline const VulkanShader &getSkyboxVertexShader() const { return skyboxVertexShader; }
	inline const VulkanShader &getSkyboxFragmentShader() const { return skyboxFragmentShader; }

	inline const VulkanTexture &getAlbedoTexture() const { return albedoTexture; }
	inline const VulkanTexture &getNormalTexture() const { return normalTexture; }
	inline const VulkanTexture &getAOTexture() const { return aoTexture; }
	inline const VulkanTexture &getShadingTexture() const { return shadingTexture; }
	inline const VulkanTexture &getEmissionTexture() const { return emissionTexture; }
	inline const VulkanTexture &getHDRTexture() const { return hdrTexture; }

	inline const VulkanMesh &getMesh() const { return mesh; }
	inline const VulkanMesh &getSkybox() const { return skybox; }

private:
	VulkanRendererContext context;

	VulkanShader pbrVertexShader;
	VulkanShader pbrFragmentShader;
	VulkanShader skyboxVertexShader;
	VulkanShader skyboxFragmentShader;

	VulkanTexture albedoTexture;
	VulkanTexture normalTexture;
	VulkanTexture aoTexture;
	VulkanTexture shadingTexture;
	VulkanTexture emissionTexture;
	VulkanTexture hdrTexture;

	VulkanMesh mesh;
	VulkanMesh skybox;
};
