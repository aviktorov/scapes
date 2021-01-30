#include <common/Log.h>
#include <render/backend/opengl/Platform.h>

#include <glad_glx.h>
#include <glad_loader.h>

namespace render::backend::opengl
{

namespace glx
{

struct Surface : public opengl::Surface
{
	GLXContext context {nullptr};
	Window window {None};
};

struct State
{
	Display *display {nullptr};
	int screen {0};
	GLXPbuffer pbuffer {None};
	GLXContext context {nullptr};
};

static State *state = nullptr;

}

//
bool Platform::init()
{
	if (glx::state)
		return true;

	glx::state = new glx::State();
	glx::state->display = XOpenDisplay(nullptr);

	if (glx::state->display == nullptr)
	{
		Log::error("opengl::Platform::init(): failed to open X display\n");
		shutdown();
		return false;
	}

	glx::state->screen = DefaultScreen(glx::state->display);

	if (!gladLoadGLXLoader(&gladLoaderProc, glx::state->display, glx::state->screen))
	{
		Log::error("opengl::Platform::init(): failed to load GLX\n");
		shutdown();
		return false;
	}

	if (!GLAD_GLX_VERSION_1_3)
	{
		Log::error("opengl::Platform::init(): invalid GLX version, GLX 1.3 is required\n");
		shutdown();
		return false;
	}

	static int visual_attribs[] =
	{
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		GLX_DOUBLEBUFFER, True,
		None
	};

	int num_configs = 0;
	GLXFBConfig *configs = glXChooseFBConfig(glx::state->display, glx::state->screen, visual_attribs, &num_configs);
	if (!configs)
	{
		Log::error("opengl::Platform::init(): failed to retrieve framebuffer configs\n");
		shutdown();
		return false;
	}

	GLXFBConfig config = configs[0];
	XFree(configs);

	static int context_attributes[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
		GLX_CONTEXT_MINOR_VERSION_ARB, 5,
		None,
	};

	glx::state->context = glXCreateContextAttribsARB(glx::state->display, config, nullptr, True, context_attributes);
	if (!glx::state->context)
	{
		Log::error("opengl::Platform::init(): failed to create dummy OpenGL context\n");
		shutdown();
		return false;
	}

	static int pbuffer_attributes[] = {
		GLX_PBUFFER_WIDTH, 1,
		GLX_PBUFFER_HEIGHT, 1,
		GL_NONE
	};

	glx::state->pbuffer = glXCreatePbuffer(glx::state->display, config, pbuffer_attributes);
	if (!glx::state->pbuffer)
	{
		Log::error("opengl::Platform::init(): failed to create dummy OpenGL surface\n");
		shutdown();
		return false;
	}

	if (glXMakeCurrent(glx::state->display, glx::state->pbuffer, glx::state->context) != True)
	{
		Log::error("opengl::Platform::init(): failed to make dummy context current\n");
		shutdown();
		return false;
	}

	return true;
}

void Platform::shutdown()
{
	if (!glx::state)
		return;

	glXMakeCurrent(glx::state->display, None, nullptr);

	glXDestroyPbuffer(glx::state->display, glx::state->pbuffer);
	glx::state->pbuffer = None;

	glXDestroyContext(glx::state->display, glx::state->context);
	glx::state->context = nullptr;

	XCloseDisplay(glx::state->display);
	glx::state->display = nullptr;
	glx::state->screen = 0;

	delete glx::state;
	glx::state = nullptr;
}

//
Surface *Platform::createSurface(void *native_window, int num_samples, bool request_debug_context)
{
	assert(glx::state);
	assert(native_window);

	static int visual_attribs[] =
	{
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		GLX_DOUBLEBUFFER, True,
		GLX_SAMPLE_BUFFERS, 1,
		GLX_SAMPLES, num_samples,
		None
	};

	int num_configs = 0;
	GLXFBConfig *configs = glXChooseFBConfig(glx::state->display, glx::state->screen, visual_attribs, &num_configs);
	if (!configs)
	{
		Log::error("opengl::Platform::createSurface(): failed to retrieve framebuffer configs\n");
		return nullptr;
	}

	GLXFBConfig config = configs[0];
	XFree(configs);

	int context_flags = 0;

	if (request_debug_context)
		context_flags |= GLX_CONTEXT_DEBUG_BIT_ARB;

	int context_attributes[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
		GLX_CONTEXT_MINOR_VERSION_ARB, 5,
		GLX_CONTEXT_FLAGS_ARB, context_flags,
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		None,
	};

	GLXContext context = glXCreateContextAttribsARB(glx::state->display, config, nullptr, True, context_attributes);
	if (!context)
	{
		Log::error("opengl::Platform::createSurface(): failed to create OpenGL 4.5 context\n");
		return nullptr;
	}

	glx::Surface *out = new glx::Surface();
	out->context = context;
	out->window = reinterpret_cast<Window>(native_window);

	return out;
}

void Platform::destroySurface(Surface *surface)
{
	assert(glx::state);

	glx::Surface *glx_surface = static_cast<glx::Surface *>(surface);
	if (!glx_surface)
		return;

	glXMakeCurrent(glx::state->display, None, nullptr);
	glXDestroyContext(glx::state->display, glx_surface->context);

	glx_surface->context = nullptr;
	glx_surface->window = None;

	delete glx_surface;
}

//
void Platform::makeCurrent(const Surface *surface)
{
	assert(glx::state);

	const glx::Surface *glx_surface = static_cast<const glx::Surface *>(surface);
	if (glx_surface)
		glXMakeCurrent(glx::state->display, glx_surface->window, glx_surface->context);
	else
		glXMakeCurrent(glx::state->display, None, nullptr);
}

void Platform::swapBuffers(const Surface *surface)
{
	assert(glx::state);
	assert(surface);

	const glx::Surface *glx_surface = static_cast<const glx::Surface *>(surface);

	glXSwapBuffers(glx::state->display, glx_surface->window);
}

}
