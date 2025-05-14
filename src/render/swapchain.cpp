#include "swapchain.h"
#include "d3d12header.h"

namespace vkr::Render
{
	SwapChain::SwapChain()
	{

	}

	SwapChain::~SwapChain()
	{

	}

	bool SwapChain::Init(IDXGIFactory2* factory, void* cmdQueue, void* nativeWindowHandle, const Vector2u& size)
	{

		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.BufferCount = 2;
		desc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.SampleDesc.Count = 1;
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		desc.Width = size.x;
		desc.Height = size.y;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
		fullscreenDesc.Windowed = true;

		factory->CreateSwapChainForHwnd((ID3D12CommandQueue*)cmdQueue, (HWND)nativeWindowHandle, &desc, &fullscreenDesc, nullptr, &m_SwapChain);

		m_SwapChain.As(&m_SwapChain4);
		return true;
	}

	void SwapChain::Present()
	{
		m_SwapChain->Present(1, 0);
	}

}