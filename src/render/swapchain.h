#pragma once
#include "rendercommon.h"

namespace vkr::Render
{
	class Texture;
	class SwapChain
	{
		static constexpr uint32_t NumBackBuffers = 2;

	public:
		SwapChain();
		~SwapChain();

		bool Init(IDXGIFactory2* factory, void* cmdQueue, void* nativeWindowHandle, const Vector2u& size);

		void Resize(const Vector2u& newSize);
		void SetHdrEnabled(bool hdr);
		bool IsHdrEnabled() const;

		void Present();

	private:
		void CreateResources();
		void ReleaseResources();

	private:
		ComPtr<IDXGISwapChain1> m_SwapChain;
		ComPtr<IDXGISwapChain4> m_SwapChain4;
		ComPtr<IDXGIOutput6> m_Output;

		struct BackBufferResource
		{
			Texture* m_Texture;
			// SRV?
			// RTV?
		};
		std::array<BackBufferResource, NumBackBuffers> m_BackBuffers;
		uint32_t currentBackBufferIndex;

		bool m_IsHdrEnabled;
		bool m_HdrSupported;
	};
}