#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "utils/types.h"
#include "utils/commandline.h"

#include "render/window.h"
#include "render/device.h"

#include "graphics/scene.h"
#include "graphics/viewrenderer.h"

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	vkr::CommandLine::Parse(__argc, __argv);
	vkr::Render::Window window = vkr::Render::Window("Raytracer Sample", {1280, 720}, nShowCmd);
	vkr::Render::Device device;

	device.Init();

	//vkr::Application app;

	/////////////////////////////////////////////////////////////////////////////////////////////
	// These things should probably be encapsulated in some form of "application" class
	vkr::Ref<vkr::Render::SwapChain> swapChain = device.CreateSwapChain(window.GetNativeHandle(), { 1280, 720 });
	vkr::Graphics::ViewRenderer viewRenderer;
	if (!viewRenderer.Init())
		return 1;

	vkr::Graphics::Scene scene;
	vkr::Ref<vkr::Graphics::View> view = scene.CreateView();
	/////////////////////////////////////////////////////////////////////////////////////////////

	// Should we encapsulate this loop to accomodate different sample apps?
	bool running = true;
	while (running)
	{
		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// app.Update();

		//////////////////////////////////////////////////////////////////////
		// These should probably also move into whatever app class we build
		//gameworld->Update()
		//view->SetCamera(updatedCameraFromGameWorld)
		scene.Update();
		scene.PrepareView(*view);
		viewRenderer.RenderView(*view);
		swapChain->Present();
		//////////////////////////////////////////////////////////////////////
	}
	return 0;
}