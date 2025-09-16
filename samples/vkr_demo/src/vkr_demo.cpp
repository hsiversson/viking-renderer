#include "core/common.h"
#include "core/types.h"
#include "application/application.h"

#include "vkr_demo_app.h"

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	char** argv = __argv;

	vkr::ApplicationInitDesc appInitDesc = {};
	appInitDesc.m_Resolution = { 1920, 1080 };
	appInitDesc.m_WindowTitle = "Viking Demo";
	appInitDesc.m_ExePath = std::filesystem::weakly_canonical(std::filesystem::path(argv[0]));
	appInitDesc.m_ContentDirectory = std::filesystem::weakly_canonical(appInitDesc.m_ExePath.parent_path() / ".." / ".." / "samples" / "vkr_demo" / "content");
	appInitDesc.m_ShowCmd = nShowCmd;
	appInitDesc.m_Mode = vkr::ApplicationMode::Editor;

	DemoApp app;
	vkr::ReturnCode result = app.Launch(appInitDesc);

	return static_cast<int32_t>(result);
}