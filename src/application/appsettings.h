#pragma once

namespace vkr
{
	enum UpscalingType : uint8_t
	{
		UPSCALING_TYPE_TAA,
		UPSCALING_TYPE_DLSS
	};

	struct TAASettings
	{
		uint32_t m_QualityMode;
	};

	enum DLSSQualityMode : uint8_t
	{
		DLSS_QUALITY_MODE_ULTRA_PERFORMANCE = 0,
		DLSS_QUALITY_MODE_MAX_PERFORMANCE,
		DLSS_QUALITY_MODE_BALANCED,
		DLSS_QUALITY_MODE_MAX_QUALITY,
		DLSS_QUALITY_MODE_DLAA
	};

	struct DLSSSettings
	{
		DLSSQualityMode m_QualityMode = DLSS_QUALITY_MODE_BALANCED;
		bool m_UseRayReconstruction = true;
	};

	struct GraphicsSettings
	{
		UpscalingType m_UpscalingType = UPSCALING_TYPE_DLSS;
		union
		{
			TAASettings m_TAA;
			DLSSSettings m_DLSS;
		};
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