
#pragma once

namespace vkr::Render
{
	class Device;

	enum NvStreamlineFeature
	{
		DLSS,
		DLSS_RR
	};

	class NvStreamline
	{
	public:
		NvStreamline();
		~NvStreamline();
		bool Init();
		bool SetDevice(const Device* device);
		bool IsFeatureAvailable(NvStreamlineFeature feature);
		bool Shutdown();
	private:
		struct PImpl;
		UniquePtr<PImpl> m_pImpl;
	};
}