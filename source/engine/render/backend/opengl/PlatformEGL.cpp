#include <common/Log.h>
#include <render/backend/opengl/Platform.h>

#include <glad_egl.h>
#include <glad_loader.h>

namespace render::backend::opengl
{

namespace egl
{

struct Surface : public opengl::Surface
{
	EGLContext context {EGL_NO_CONTEXT};
	EGLSurface surface {EGL_NO_SURFACE};
};

struct State
{
	EGLDisplay display {EGL_NO_DISPLAY};
	EGLContext dummy_context {EGL_NO_CONTEXT};
	EGLSurface dummy_surface {EGL_NO_SURFACE};
};

static State *state = nullptr;

}

//
bool Platform::init()
{
	if (egl::state)
		return true;

	if (!gladLoadEGLLoader(&gladLoaderProc))
	{
		Log::error("opengl::Platform::init(): failed to load EGL\n");
		return false;
	}

	egl::state = new egl::State();

	egl::state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(egl::state->display != EGL_NO_DISPLAY);

	EGLint major, minor;
	if (!eglInitialize(egl::state->display, &major, &minor))
	{
		Log::error("opengl::Platform::init(): failed to initialize EGL\n");
		shutdown();
		return false;
	}

	static EGLint config_attribs[] =
	{
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_NONE,
	};

	EGLint num_configs = 0;
	EGLConfig config = nullptr;
	
	if (!eglChooseConfig(egl::state->display, config_attribs, &config, 1, &num_configs))
	{
		Log::error("opengl::Platform::init(): failed to choose EGL config\n");
		shutdown();
		return false;
	}

	static int context_attribs[] =
	{
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_NONE,
	};

	egl::state->dummy_context = eglCreateContext(egl::state->display, config, nullptr, context_attribs);
	if (egl::state->dummy_context == EGL_NO_CONTEXT)
	{
		Log::error("opengl::Platform::init(): failed to create dummy OpenGL ES 3.0 context\n");
		shutdown();
		return false;
	}

	static int pbuffer_attribs[] =
	{
		EGL_WIDTH, 1,
		EGL_HEIGHT, 1,
		EGL_NONE,
	};

	egl::state->dummy_surface = eglCreatePbufferSurface(egl::state->display, config, pbuffer_attribs);
	if (egl::state->dummy_surface == EGL_NO_SURFACE)
	{
		Log::error("opengl::Platform::init(): failed to create dummy surface\n");
		shutdown();
		return false;
	}

	if (!eglMakeCurrent(egl::state->display, egl::state->dummy_surface, egl::state->dummy_surface, egl::state->dummy_context))
	{
		Log::error("opengl::Platform::init(): failed to make dummy context current\n");
		shutdown();
		return false;
	}

	return true;
}

void Platform::shutdown()
{
	if (!egl::state)
		return;

	eglMakeCurrent(egl::state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	if (egl::state->dummy_context)
		eglDestroyContext(egl::state->display, egl::state->dummy_context);

	if (egl::state->dummy_surface)
		eglDestroySurface(egl::state->display, egl::state->dummy_surface);

	eglTerminate(egl::state->display);
	eglReleaseThread();

	egl::state->display = EGL_NO_DISPLAY;
	egl::state->dummy_context = EGL_NO_CONTEXT;
	egl::state->dummy_surface = EGL_NO_SURFACE;
	
	delete egl::state;
	egl::state = nullptr;
}

//
Surface *Platform::createSurface(void *native_window, int num_samples, bool request_debug_context)
{
	assert(egl::state);
	assert(native_window);

	int context_flags = 0;

	if (request_debug_context)
		context_flags |= EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;

	static EGLint config_attribs[] =
	{
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_SAMPLE_BUFFERS, 1,
		EGL_SAMPLES, num_samples,
		EGL_NONE,
	};

	EGLint num_configs = 0;
	EGLConfig config = nullptr;
	
	if (!eglChooseConfig(egl::state->display, config_attribs, &config, 1, &num_configs))
	{
		Log::error("opengl::Platform::createSurface(): failed to choose EGL config\n");
		return nullptr;
	}

	int context_attribs[] =
	{
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 1,
		EGL_CONTEXT_FLAGS_KHR, context_flags,
		EGL_NONE,
	};

	EGLContext context = eglCreateContext(egl::state->display, config, nullptr, context_attribs);
	if (context == EGL_NO_CONTEXT)
	{
		Log::error("opengl::Platform::createSurface(): failed to create OpenGL ES 3.1 context\n");
		return nullptr;
	}

	EGLSurface surface = eglCreateWindowSurface(egl::state->display, config, reinterpret_cast<EGLNativeWindowType>(native_window), nullptr);
	if (surface == EGL_NO_SURFACE)
	{
		Log::error("opengl::Platform::createSurface(): failed to create EGL surface\n");
		eglDestroyContext(egl::state->display, context);
		return nullptr;
	}

	if (!eglSurfaceAttrib(egl::state->display, surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED))
		Log::warning("opengl::Platform::createSurface(): failed to set EGL_SWAP_BEHAVIOR on surface\n");

	egl::Surface *out = new egl::Surface();
	out->surface = surface;
	out->context = context;

	return out;
}

void Platform::destroySurface(Surface *surface)
{
	assert(egl::state);

	egl::Surface *egl_surface = static_cast<egl::Surface *>(surface);
	if (!egl_surface)
		return;

	eglMakeCurrent(egl::state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	if (egl_surface->context)
		eglDestroyContext(egl::state->display, egl_surface->context);

	if (egl_surface->surface)
		eglDestroySurface(egl::state->display, egl_surface->surface);

	egl_surface->context = nullptr;
	egl_surface->surface = nullptr;

	delete egl_surface;
}

//
void Platform::makeCurrent(const Surface *surface)
{
	assert(egl::state);

	const egl::Surface *egl_surface = static_cast<const egl::Surface *>(surface);
	if (egl_surface)
		eglMakeCurrent(egl::state->display, egl_surface->surface, egl_surface->surface, egl_surface->context);
	else
		eglMakeCurrent(egl::state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void Platform::swapBuffers(const Surface *surface)
{
	assert(egl::state);

	const egl::Surface *egl_surface = static_cast<const egl::Surface *>(surface);
	eglSwapBuffers(egl::state->display, egl_surface->surface);
}

}
