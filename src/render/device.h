#pragma once

#include "context.h"
#include "d3d12header.h"
#include "swapchain.h"
#include "utils/types.h"

namespace vkr::Render
{
	class Device
	{
	public:
		Device();
		~Device();

		bool Init(bool enableDebugLayer = false);

		// Create render resources (textures, buffers, PSOs...)
		SwapChain* CreateSwapChain(void* windowHandle, const Vector2u& size);
		Context* CreateContext();

	private:
		ComPtr<IDXGIFactory2> m_Factory;
		ComPtr<IDXGIAdapter1> m_Adapter;
		ComPtr<ID3D12Device> m_Device;
		ComPtr<ID3D12CommandQueue> m_CommandQueue;
	};
}

