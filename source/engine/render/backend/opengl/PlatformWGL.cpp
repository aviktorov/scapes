#include <common/Log.h>
#include <render/backend/opengl/Platform.h>

#include <glad_wgl.h>
#include <glad_loader.h>

#include <cassert>

namespace render::backend::opengl
{

namespace wgl
{

struct Surface : public opengl::Surface
{
	HGLRC context {nullptr};
	HWND window {nullptr};
	HDC dc {nullptr};
};

struct State
{
	HGLRC dummy_context {nullptr};
	HWND dummy_window {nullptr};
	HDC dummy_dc {nullptr};
};

static State *state = nullptr;

}

//
bool Platform::init()
{
	if (wgl::state)
		return true;

	PIXELFORMATDESCRIPTOR pfd = {};
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	wgl::state = new wgl::State();
	wgl::state->dummy_window = CreateWindowA("STATIC", "Dummy", 0, 0, 0, 1, 1, nullptr, nullptr, nullptr, nullptr);
	wgl::state->dummy_dc = GetDC(wgl::state->dummy_window);

	if (wgl::state->dummy_dc == nullptr)
	{
		Log::error("opengl::Platform::init(): failed to create dummy window for WGL\n");
		shutdown();
		return false;
	}

	int pixel_format = ChoosePixelFormat(wgl::state->dummy_dc, &pfd);
	SetPixelFormat(wgl::state->dummy_dc, pixel_format, nullptr);

	wgl::state->dummy_context = wglCreateContext(wgl::state->dummy_dc);
	if (!wglMakeCurrent(wgl::state->dummy_dc, wgl::state->dummy_context))
	{
		Log::error("opengl::Platform::init(): failed to make dummy context current\n");
		shutdown();
		return false;
	}

	if (!gladLoadWGLLoader(&gladLoaderProc, wgl::state->dummy_dc))
	{
		Log::error("opengl::Platform::init(): failed to load WGL\n");
		shutdown();
		return false;
	}

	return true;
}

void Platform::shutdown()
{
	if (!wgl::state)
		return;

	wglMakeCurrent(nullptr, nullptr);
	wglDeleteContext(wgl::state->dummy_context);

	ReleaseDC(wgl::state->dummy_window, wgl::state->dummy_dc);
	DestroyWindow(wgl::state->dummy_window);

	wgl::state->dummy_context = nullptr;
	wgl::state->dummy_window = nullptr;
	wgl::state->dummy_dc = nullptr;

	delete wgl::state;
	wgl::state = nullptr;
}

//
Surface *Platform::createSurface(void *native_window, int num_samples, bool request_debug_context)
{
	assert(wgl::state);
	assert(native_window);

	HWND window = reinterpret_cast<HWND>(native_window);
	HDC dc = GetDC(window);
	assert(dc);

	static int config_attribs[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_SAMPLE_BUFFERS_ARB, 1,
		WGL_SAMPLES_ARB, num_samples,
		0,
	};

	UINT num_formats = 0;
	int pixel_format = 0;
	
	if (!wglChoosePixelFormatARB(dc, config_attribs, nullptr, 1, &pixel_format, &num_formats))
	{
		Log::error("opengl::Platform::createSurface(): failed to choose pixel format\n");
		shutdown();
		return false;
	}
	SetPixelFormat(dc, pixel_format, nullptr);

	int context_flags = 0;

	if (request_debug_context)
		context_flags |= WGL_CONTEXT_DEBUG_BIT_ARB;

	int context_attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 5,
		WGL_CONTEXT_FLAGS_ARB, context_flags,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	HGLRC context = wglCreateContextAttribsARB(dc, nullptr, context_attributes);
	if (context == nullptr)
	{
		Log::error("opengl::Platform::createSurface(): failed to create OpenGL 4.5 context\n");
		ReleaseDC(window, dc);
		return nullptr;
	}

	wgl::Surface *out = new wgl::Surface();
	out->window = window;
	out->dc = dc;
	out->context = context;

	return out;
}

void Platform::destroySurface(Surface *surface)
{
	assert(wgl::state);

	wgl::Surface *wgl_surface = static_cast<wgl::Surface *>(surface);
	if (!wgl_surface)
		return;

	wglMakeCurrent(nullptr, nullptr);

	if (wgl_surface->context)
		wglDeleteContext(wgl_surface->context);

	if (wgl_surface->dc)
		ReleaseDC(wgl_surface->window, wgl_surface->dc);

	wgl_surface->window = nullptr;
	wgl_surface->dc = nullptr;
	wgl_surface->context = nullptr;

	delete wgl_surface;
}

//
void Platform::makeCurrent(const Surface *surface)
{
	assert(wgl::state);

	const wgl::Surface *wgl_surface = static_cast<const wgl::Surface *>(surface);
	if (wgl_surface)
		wglMakeCurrent(wgl_surface->dc, wgl_surface->context);
	else
		wglMakeCurrent(nullptr, nullptr);
	
}

void Platform::swapBuffers(const Surface *surface)
{
	assert(wgl::state);

	const wgl::Surface *wgl_surface = static_cast<const wgl::Surface *>(surface);
	SwapBuffers(wgl_surface->dc);
}

}
