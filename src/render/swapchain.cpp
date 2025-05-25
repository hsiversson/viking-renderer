#include "swapchain.h"
#include "device.h"
#include "commandqueue.h"

namespace vkr::Render
{
	SwapChain::SwapChain(Device& device)
		: DeviceObject(device)
		, m_CurrentBackBufferIndex(0)
		, m_IsHdrEnabled(false)
		, m_HdrSupported(false)
	{
	}

	SwapChain::~SwapChain()
	{
		ReleaseResources();
	}

	bool SwapChain::Init(void* nativeWindowHandle, const Vector2u& size)
	{
		m_CommandQueue = m_Device.GetCommandQueue(CONTEXT_TYPE_PRESENT);

		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.BufferCount = NumBackBuffers;
		desc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.SampleDesc.Count = 1;
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		desc.Width = size.x;
		desc.Height = size.y;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
		fullscreenDesc.Windowed = true;

		m_Device.GetDXGIFactory()->CreateSwapChainForHwnd(m_CommandQueue->GetD3DCommandQueue(), (HWND)nativeWindowHandle, &desc, &fullscreenDesc, nullptr, &m_SwapChain);
		m_SwapChain.As(&m_SwapChain4);

		ComPtr<IDXGIOutput> output;
		m_SwapChain4->GetContainingOutput(&output);
		output.As(&m_Output);

		DXGI_OUTPUT_DESC1 outputDesc = {};
		m_Output->GetDesc1(&outputDesc);
		m_HdrSupported = (outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);

		CreateResources();
		SetHdrEnabled(false);
		return true;
	}

	void SwapChain::Resize(const Vector2u& newSize)
	{
		ReleaseResources();

		m_SwapChain->ResizeBuffers(NumBackBuffers, newSize.x, newSize.y, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

		CreateResources();
	}

	void SwapChain::SetHdrEnabled(bool hdr)
	{
		if (m_IsHdrEnabled != hdr)
		{
			DXGI_COLOR_SPACE_TYPE targetColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
			if (hdr && m_HdrSupported)
				targetColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;

			m_SwapChain4->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
			m_IsHdrEnabled = hdr;
		}
	}

	bool SwapChain::IsHdrEnabled() const
	{
		return m_IsHdrEnabled;
	}

	void SwapChain::Present()
	{
		m_SwapChain->Present(1, 0);
		m_BackBuffers[m_CurrentBackBufferIndex].m_LastFrameEvent = m_CommandQueue->Signal();
		m_CurrentBackBufferIndex = m_SwapChain4->GetCurrentBackBufferIndex();

		// Wait for current backbuffer to become available
		m_BackBuffers[m_CurrentBackBufferIndex].m_LastFrameEvent.Wait();
	}

	void SwapChain::CreateResources()
	{
	}

	void SwapChain::ReleaseResources()
	{
		// Wait for GPU
	}

}