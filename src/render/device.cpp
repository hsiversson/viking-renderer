#include "device.h"
#include "shadercompiler.h"
#include "rootsignature.h"

#include <algorithm>

namespace vkr::Render
{
	static const D3D12_HEAP_PROPERTIES DefHeapProps{ D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
	static const D3D12_HEAP_PROPERTIES UploadHeapProps{ D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };

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

		InitRootSignatures();

		return true;
	}

	void Device::InitRootSignatures()
	{
		for (int i = 0; i < PipelineStateType::PIPELINE_STATE_TYPE_COUNT; i++)
		{
			auto Signature = new RootSignature;
			// For now just consider one unique constant buffer?
			Signature->Init({ PipelineStateType(i), 1}, m_Device.Get());
			m_RootSignatures[i] = Signature;
		}
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
		if (!pipelineState->Init(desc, m_RootSignatures[desc.m_Type], m_Device.Get()))
		{
			delete pipelineState;
			return nullptr;
		}
		return pipelineState;
	}

	Texture* Device::CreateTexture(const TextureDesc& desc)
	{
		Texture* texture = new Texture;

		//For now allocate resource in place. Later well see how we do pooling
		UINT16 MipLevels = 1;
		if (desc.bUseMips)
		{
			int32_t MaxDim = std::max<int32_t>(desc.Size.x, desc.Size.y);
			MipLevels = unsigned int(std::floor(std::log2(MaxDim))) + 1;
		}

		ID3D12Resource* resource;
		D3D12_RESOURCE_DESC TextureDesc;
		TextureDesc.Dimension = desc.Dimension == 1 ? D3D12_RESOURCE_DIMENSION_TEXTURE1D : (desc.Dimension == 2 ? D3D12_RESOURCE_DIMENSION_TEXTURE2D : D3D12_RESOURCE_DIMENSION_TEXTURE3D);
		TextureDesc.Format = desc.Format;
		TextureDesc.MipLevels = MipLevels;
		TextureDesc.Alignment = 0;
		TextureDesc.DepthOrArraySize = desc.ArraySize;
		TextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		TextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		HRESULT hr = m_Device->CreateCommittedResource(&DefHeapProps, D3D12_HEAP_FLAG_NONE, &TextureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource));
		if (FAILED(hr))
		{
			return nullptr;
		}

		if (!texture->Init(resource))
		{
			delete texture;
			return nullptr;
		}
		return texture;
	}

	Buffer* Device::CreateBuffer(const BufferDesc& desc)
	{
		Buffer* buffer = new Buffer;

		//For now allocate resource in place. Later well see how we do pooling

		ID3D12Resource* resource;
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = 0;
		bufferDesc.Width = desc.Size;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; 
		bufferDesc.Flags = desc.bWriteOnGPU ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

		HRESULT hr = m_Device->CreateCommittedResource(desc.bWriteOnCPU ? &UploadHeapProps : &DefHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource));
		if (FAILED(hr))
		{
			return nullptr;
		}

		if (!buffer->Init(resource))
		{
			delete buffer;
			return nullptr;
		}
		return buffer;
	}

}