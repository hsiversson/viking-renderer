#include "core/common.h"
#include "raytracer_app.h"

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	vkr::ApplicationInitDesc appInitDesc = {};
	appInitDesc.m_Resolution = { 1280, 720 };
	appInitDesc.m_WindowTitle = "Raytracer Sample";
	appInitDesc.m_ShowCmd = nShowCmd;
	appInitDesc.m_Mode = vkr::ApplicationMode::Editor;

	RaytracerApp app;
	vkr::ReturnCode result = app.Launch(appInitDesc);

	return static_cast<int32_t>(result);
}