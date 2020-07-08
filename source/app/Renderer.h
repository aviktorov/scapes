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

	void setEnvironment(const ApplicationResources *scene, int index);

private:
	render::backend::Driver *driver {nullptr};
	render::backend::BindSet *scene_bind_set {nullptr};
};
