#pragma once

#include <render/backend/Driver.h>

namespace render
{
	struct RenderFrame;
	class SwapChain;
	class Texture;
}

struct ImGuiContext;

/*
 */
class ImGuiRenderer
{
public:
	ImGuiRenderer(
		render::backend::Driver *driver,
		ImGuiContext *imguiContext
	);

	virtual ~ImGuiRenderer();

	void *addTexture(const render::Texture *texture) const;

	void init(const render::SwapChain *swap_chain);
	void shutdown();
	void resize(const render::SwapChain *swap_chain);
	void render(const render::RenderFrame &frame);

private:
	render::backend::Driver *driver {nullptr};
	ImGuiContext *imguiContext {nullptr};
};
