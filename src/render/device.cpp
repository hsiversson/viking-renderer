#include "device.h"
#include "shadercompiler.h"
#include "rootsignature.h"

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

		m_ShaderCompiler = new ShaderCompiler;
		return true;
	}

	vkr::Render::Context* Device::CreateContext()
	{
		ID3D12CommandAllocator* commandAllocator = nullptr;
		if (FAILED(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator))))
		{
			return nullptr;
		}
		ID3D12GraphicsCommandList* commandList = nullptr;
		if (FAILED(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList))))
		{
			commandAllocator->Release();
			commandAllocator = nullptr;
			return nullptr;
		}
		commandList->Close();

		Context* context = new Context;
		context->Init(commandList, commandAllocator);
		return context;
	}

	vkr::Render::SwapChain* Device::CreateSwapChain(void* windowHandle, const Vector2u& size)
	{
		SwapChain* swapChain = new SwapChain;
		if (!swapChain->Init(m_Factory.Get(), m_CommandQueue.Get(), windowHandle, size))
		{
			delete swapChain;
			return nullptr;
		}

		return swapChain;
	}

	Shader* Device::CreateShader(const std::filesystem::path& filepath, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel)
	{
		Shader* shader = new Shader;
		if (!m_ShaderCompiler->CompileFromFile(*shader, filepath, entryPoint, stage, shaderModel))
		{
			delete shader;
			return nullptr;
		}
		return shader;
	}

	PipelineState* Device::CreatePipelineState(const PipelineStateDesc& desc)
	{
		PipelineState* pipelineState = new PipelineState;
		if (!pipelineState->Init(desc, m_RootSignatures[desc.m_Type]))
		{
			delete pipelineState;
			return nullptr;
		}
		return pipelineState;
	}

	Texture* Device::CreateTexture(const TextureDesc& desc)
	{
		Texture* texture = new Texture;
		if (!texture->Init(desc))
		{
			delete texture;
			return nullptr;
		}
		return texture;
	}

	Buffer* Device::CreateBuffer(const BufferDesc& desc)
	{
		Buffer* buffer = new Buffer;
		if (!buffer->Init(desc))
		{
			delete buffer;
			return nullptr;
		}
		return buffer;
	}

}