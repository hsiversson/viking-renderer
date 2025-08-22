#pragma once

namespace vkr
{
	enum AntialiasingMethod
	{
		TAA,
		DLSS
	};

	enum DLSSMode
	{
		UltraPerformance = 0,
		MaxPerformance,
		Balanced,
		MaxQuality,
		DLAA
	};

	struct GraphicsSettings
	{
		AntialiasingMethod m_AAMethod = AntialiasingMethod::DLSS;
		DLSSMode m_DLSSMode = DLSSMode::Balanced;
	};

	class AppSettings
	{
	public:
		AppSettings();
		GraphicsSettings& GetGraphicsSettings();
		static AppSettings* GetAppSettings();
	private:
		GraphicsSettings m_GraphicsSettings;
		static AppSettings* g_Instance;
	};
}