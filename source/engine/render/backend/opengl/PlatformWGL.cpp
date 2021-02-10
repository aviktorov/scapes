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
	HWND window {nullptr};
	HDC dc {nullptr};
};

struct State
{
	HGLRC context {nullptr};
	HGLRC dummy_context {nullptr};
	HWND dummy_window {nullptr};
	HDC dummy_dc {nullptr};
	PIXELFORMATDESCRIPTOR pfd {};
};

static State *state = nullptr;

}

//
bool Platform::init(bool request_debug_context)
{
	if (wgl::state)
		return true;

	wgl::state = new wgl::State();
	wgl::state->dummy_window = CreateWindowA("STATIC", "Dummy", 0, 0, 0, 1, 1, nullptr, nullptr, nullptr, nullptr);
	wgl::state->dummy_dc = GetDC(wgl::state->dummy_window);

	wgl::state->pfd = {};
	wgl::state->pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	wgl::state->pfd.nVersion = 1;
	wgl::state->pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	wgl::state->pfd.iPixelType = PFD_TYPE_RGBA;
	wgl::state->pfd.cColorBits = 32;
	wgl::state->pfd.cDepthBits = 24;
	wgl::state->pfd.cStencilBits = 8;
	wgl::state->pfd.iLayerType = PFD_MAIN_PLANE;

	if (wgl::state->dummy_dc == nullptr)
	{
		Log::error("opengl::Platform::init(): failed to create dummy window for WGL\n");
		shutdown();
		return false;
	}

	int pixel_format = ChoosePixelFormat(wgl::state->dummy_dc, &wgl::state->pfd);
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

	wgl::state->context = wglCreateContextAttribsARB(wgl::state->dummy_dc, nullptr, context_attributes);
	if (wgl::state->context == nullptr)
	{
		Log::error("opengl::Platform::init(): failed to create OpenGL 4.5 context\n");
		shutdown();
		return false;
	}

	if (!wglMakeCurrent(wgl::state->dummy_dc, wgl::state->context))
	{
		Log::error("opengl::Platform::init(): failed to make context current\n");
		shutdown();
		return false;
	}

	wglDeleteContext(wgl::state->dummy_context);
	wgl::state->dummy_context = nullptr;

	return true;
}

void Platform::shutdown()
{
	if (!wgl::state)
		return;

	wglMakeCurrent(nullptr, nullptr);
	wglDeleteContext(wgl::state->context);
	wglDeleteContext(wgl::state->dummy_context);

	ReleaseDC(wgl::state->dummy_window, wgl::state->dummy_dc);
	DestroyWindow(wgl::state->dummy_window);

	wgl::state->context = nullptr;
	wgl::state->dummy_context = nullptr;
	wgl::state->dummy_window = nullptr;
	wgl::state->dummy_dc = nullptr;
	wgl::state->pfd = {};

	delete wgl::state;
	wgl::state = nullptr;
}

//
Surface *Platform::createSurface(void *native_window)
{
	assert(wgl::state);
	assert(native_window);

	HWND window = reinterpret_cast<HWND>(native_window);
	HDC dc = GetDC(window);
	assert(dc);

	int pixel_format = ChoosePixelFormat(dc, &wgl::state->pfd);
	SetPixelFormat(dc, pixel_format, nullptr);

	wgl::Surface *out = new wgl::Surface();
	out->window = window;
	out->dc = dc;

	return out;
}

void Platform::destroySurface(Surface *surface)
{
	assert(wgl::state);

	wgl::Surface *wgl_surface = static_cast<wgl::Surface *>(surface);
	if (!wgl_surface)
		return;

	if (wgl_surface->dc)
		ReleaseDC(wgl_surface->window, wgl_surface->dc);

	wgl_surface->window = nullptr;
	wgl_surface->dc = nullptr;

	delete wgl_surface;

	wglMakeCurrent(wgl::state->dummy_dc, wgl::state->context);
}

//
void Platform::makeCurrent(const Surface *surface)
{
	assert(wgl::state);

	const wgl::Surface *wgl_surface = static_cast<const wgl::Surface *>(surface);
	if (wgl_surface)
		wglMakeCurrent(wgl_surface->dc, wgl::state->context);
	else
		wglMakeCurrent(nullptr, nullptr);
	
}

void Platform::swapBuffers(const Surface *surface)
{
	assert(wgl::state);
	assert(surface);

	const wgl::Surface *wgl_surface = static_cast<const wgl::Surface *>(surface);
	assert(wgl_surface->dc);

	SwapBuffers(wgl_surface->dc);
}

}
