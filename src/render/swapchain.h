#pragma once
#include "utils/types.h"
#include "d3d12header.h"

namespace vkr::Render
{
	class SwapChain
	{
	public:
		SwapChain();
		~SwapChain();

		bool Init(IDXGIFactory2* factory, void* cmdQueue, void* nativeWindowHandle, const Vector2u& size);

		void Present();

	private:
		ComPtr<IDXGISwapChain1> m_SwapChain;
		ComPtr<IDXGISwapChain4> m_SwapChain4;
	};
}