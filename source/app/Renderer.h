#pragma once

#include <vector>

#include "CubemapRenderer.h"
#include "Texture2DRenderer.h"

#include <render/Texture.h>
#include <render/Shader.h>
#include <render/backend/driver.h>

class ApplicationResources;

namespace render
{
	class SwapChain;
	struct RenderFrame;
}

/*
 */
class Renderer
{
public:
	Renderer(render::backend::Driver *driver);
	virtual ~Renderer();

	void init(const ApplicationResources *scene);
	void shutdown();
	void resize(const render::SwapChain *swapChain);
	void render(const ApplicationResources *scene, const render::RenderFrame &frame);

	void setEnvironment(const ApplicationResources *scene, const render::Texture *texture);

private:
	render::backend::Driver *driver {nullptr};

	Texture2DRenderer bakedBRDFRenderer;
	CubemapRenderer hdriToCubeRenderer;
	std::vector<CubemapRenderer*> cubeToPrefilteredRenderers;
	CubemapRenderer diffuseIrradianceRenderer;

	render::Texture bakedBRDF;
	render::Texture environmentCubemap;
	render::Texture diffuseIrradianceCubemap;

	render::backend::BindSet *scene_bind_set {nullptr};
};
