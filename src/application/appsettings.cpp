#include "appsettings.h"

namespace vkr
{
	AppSettings* AppSettings::g_Instance = nullptr;

	AppSettings::AppSettings()
		: m_GraphicsSettings{}
	{
		VKR_ASSERT(g_Instance == nullptr);
		g_Instance = this;

		// Only inited here because of the union, we should have a default function setting all the defaults and then save/load from disk settings.
		m_GraphicsSettings.m_DLSS.m_QualityMode = DLSS_QUALITY_MODE_BALANCED;
		m_GraphicsSettings.m_DLSS.m_UseRayReconstruction = true;
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