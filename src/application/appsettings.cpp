#include "appsettings.h"

namespace vkr
{
	AppSettings* AppSettings::g_Instance = nullptr;

	AppSettings::AppSettings()
	{
		VKR_ASSERT(g_Instance == nullptr);
		g_Instance = this;
	}

	vkr::GraphicsSettings& AppSettings::GetGraphicsSettings()
	{
		return m_GraphicsSettings;
	}

	vkr::AppSettings* AppSettings::GetAppSettings()
	{
		return g_Instance;
	}

}