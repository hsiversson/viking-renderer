
#pragma once

namespace vkr::Render
{
	class Device;

	class NvStreamline
	{
	public:
		NvStreamline();
		~NvStreamline();
		bool Init();
		bool SetDevice(const Device* device);
		bool Shutdown();
	private:
		struct PImpl;
		UniquePtr<PImpl> m_pImpl;
	};
}