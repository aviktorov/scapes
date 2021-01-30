#pragma once

namespace render::backend::opengl
{

struct Surface {};

class Platform
{
public:
	static bool init();
	static void shutdown();

	static Surface *createSurface(void *native_window, int num_samples, bool request_debug_context);
	static void destroySurface(Surface *surface);

	static void makeCurrent(const Surface *surface);
	static void swapBuffers(const Surface *surface);
};

}
