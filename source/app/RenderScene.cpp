#include "RenderScene.h"

/*
 */
void RenderScene::init(
	const std::string &vertexShaderFile,
	const std::string &fragmentShaderFile,
	const std::string &textureFile,
	const std::string &modelFile
)
{
	vertexShader.loadFromFile(vertexShaderFile);
	fragmentShader.loadFromFile(fragmentShaderFile);
	mesh.loadFromFile(modelFile);
	texture.loadFromFile(textureFile);
}

void RenderScene::shutdown()
{
	texture.clearGPUData();
	mesh.clearGPUData();
	vertexShader.clear();
	fragmentShader.clear();
}
