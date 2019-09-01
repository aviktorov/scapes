#include "RenderScene.h"

/*
 */
void RenderScene::init(
	const std::string &vertexShaderFile,
	const std::string &fragmentShaderFile,
	const std::string &albedoFile,
	const std::string &normalFile,
	const std::string &aoFile,
	const std::string &shadingFile,
	const std::string &emissionFile,
	const std::string &modelFile
)
{
	vertexShader.compileFromFile(vertexShaderFile, VulkanShaderKind::Vertex);
	fragmentShader.compileFromFile(fragmentShaderFile, VulkanShaderKind::Fragment);
	mesh.loadFromFile(modelFile);
	albedoTexture.loadFromFile(albedoFile);
	normalTexture.loadFromFile(normalFile);
	aoTexture.loadFromFile(aoFile);
	shadingTexture.loadFromFile(shadingFile);
	emissionTexture.loadFromFile(emissionFile);
}

void RenderScene::shutdown()
{
	albedoTexture.clearGPUData();
	normalTexture.clearGPUData();
	aoTexture.clearGPUData();
	shadingTexture.clearGPUData();
	emissionTexture.clearGPUData();
	mesh.clearGPUData();
	vertexShader.clear();
	fragmentShader.clear();
}
