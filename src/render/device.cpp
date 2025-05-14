#include "device.h"

namespace vkr::Render
{
	Device::Device()
	{

	}

	Device::~Device()
	{

	}

	bool Device::Init(bool enableDebugLayer)
	{
		uint32_t createFactoryFlags = 0;
		if (enableDebugLayer)
			createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&m_Factory));

		m_Factory->EnumAdapters1(0, &m_Adapter); // Make this smarter?

		if (enableDebugLayer)
		{
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
			}
		}

		D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device));

		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		m_Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_CommandQueue));
		return true;
	}

	vkr::Render::SwapChain* Device::CreateSwapChain(void* windowHandle, const Vector2u& size)
	{
		SwapChain* swapChain = new SwapChain();
		if (!swapChain->Init(m_Factory.Get(), m_CommandQueue.Get(), windowHandle, size))
		{
			delete swapChain;
			return nullptr;
		}

		return swapChain;
	}

}