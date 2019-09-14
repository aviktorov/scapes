#include "RenderScene.h"

/*
 */
void RenderScene::init(
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
)
{
	pbrVertexShader.compileFromFile(pbrVertexShaderFile, VulkanShaderKind::Vertex);
	pbrFragmentShader.compileFromFile(pbrFragmentShaderFile, VulkanShaderKind::Fragment);
	skyboxVertexShader.compileFromFile(skyboxVertexShaderFile, VulkanShaderKind::Vertex);
	skyboxFragmentShader.compileFromFile(skyboxFragmentShaderFile, VulkanShaderKind::Fragment);

	albedoTexture.loadFromFile(albedoFile);
	normalTexture.loadFromFile(normalFile);
	aoTexture.loadFromFile(aoFile);
	shadingTexture.loadFromFile(shadingFile);
	emissionTexture.loadFromFile(emissionFile);
	hdrTexture.loadHDRFromFile(hdrFile);

	mesh.loadFromFile(modelFile);
	skybox.createSkybox(100.0f);
}

void RenderScene::shutdown()
{
	pbrVertexShader.clear();
	pbrFragmentShader.clear();
	skyboxVertexShader.clear();
	skyboxFragmentShader.clear();

	albedoTexture.clearGPUData();
	normalTexture.clearGPUData();
	aoTexture.clearGPUData();
	shadingTexture.clearGPUData();
	emissionTexture.clearGPUData();
	hdrTexture.clearGPUData();

	mesh.clearGPUData();
}
