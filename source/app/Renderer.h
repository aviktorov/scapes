#pragma once

class ApplicationResources;
class SkyLight;

namespace render
{
	struct RenderFrame;
}

namespace render::backend
{
	class Driver;
	struct BindSet;
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
	void render(const ApplicationResources *scene, const SkyLight *light, const render::RenderFrame &frame);

private:
	render::backend::Driver *driver {nullptr};
	render::backend::BindSet *scene_bind_set {nullptr};
};
