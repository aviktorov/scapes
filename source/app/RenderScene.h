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
		: context(context), mesh(context), texture(context), vertexShader(context), fragmentShader(context) { }

	void init(
		const std::string &vertexShaderFile,
		const std::string &fragmentShaderFile,
		const std::string &textureFile,
		const std::string &modelFile
	);
	void shutdown();

	inline const VulkanTexture &getTexture() const { return texture; }
	inline const VulkanMesh &getMesh() const { return mesh; }
	inline const VulkanShader &getVertexShader() const { return vertexShader; }
	inline const VulkanShader &getFragmentShader() const { return fragmentShader; }

private:
	VulkanRendererContext context;

	VulkanMesh mesh;
	VulkanTexture texture;
	VulkanShader vertexShader;
	VulkanShader fragmentShader;
};
