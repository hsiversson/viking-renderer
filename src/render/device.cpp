#include "device.h"
#include "shadercompiler.h"
#include "rootsignature.h"
#include "textureloader_dds.h"
#include "textureloader_png.h"
#include "textureloader_tga.h"

#include "utils/commandline.h"

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

	bool Device::Init()
	{
		const bool enableDebugLayer = CommandLine::Has("debug_device");

		uint32_t createFactoryFlags = (enableDebugLayer) ? DXGI_CREATE_FACTORY_DEBUG : 0;
		CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&m_Factory));

		if (enableDebugLayer)
		{
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
			}
		}

		m_Factory->EnumAdapters1(0, &m_Adapter); // Make this smarter?

		D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device));

		m_ShaderCompiler = new ShaderCompiler;

		InitRootSignatures();
		InitTextureLoaders();
		InitCommandQueues();
		return true;
	}

	void Device::InitRootSignatures()
	{
		for (int i = 0; i < PipelineStateType::PIPELINE_STATE_TYPE_COUNT; i++)
		{
			auto Signature = new RootSignature(*this);
			// For now just consider one unique constant buffer?
			Signature->Init({ PipelineStateType(i), 1 });
			m_RootSignatures[i] = Signature;
		}
	}

	vkr::Render::Context* Device::CreateContext(ContextType contextType)
	{
		// Should we move this into the context itself.
		// Potentially each context would need several command lists, 
		// so we might need some form of functionality to "reset" a context with a new command list.
		// Maybe Context::Begin & Context::End functions?

		D3D12_COMMAND_LIST_TYPE type;
		switch (contextType)
		{
		case vkr::Render::CONTEXT_TYPE_GRAPHICS:
			type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			break;
		case vkr::Render::CONTEXT_TYPE_COMPUTE:
			type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			break;
		case vkr::Render::CONTEXT_TYPE_COPY:
			type = D3D12_COMMAND_LIST_TYPE_COPY;
			break;
		default:
			assert(false);
			return nullptr;
		}

		ID3D12CommandAllocator* commandAllocator = nullptr;
		if (FAILED(m_Device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator))))
		{
			return nullptr;
		}
		ID3D12GraphicsCommandList* commandList = nullptr;
		if (FAILED(m_Device->CreateCommandList(0, type, commandAllocator, nullptr, IID_PPV_ARGS(&commandList))))
		{
			commandAllocator->Release();
			commandAllocator = nullptr;
			return nullptr;
		}
		commandList->Close();

		Context* context = new Context(*this, contextType);
		context->Init(commandList, commandAllocator);
		return context;
	}

	vkr::Render::SwapChain* Device::CreateSwapChain(void* windowHandle, const Vector2u& size)
	{
		SwapChain* swapChain = new SwapChain(*this);
		if (!swapChain->Init(windowHandle, size))
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
		PipelineState* pipelineState = new PipelineState(*this);
		if (!pipelineState->Init(desc, m_RootSignatures[desc.m_Type]))
		{
			delete pipelineState;
			return nullptr;
		}
		return pipelineState;
	}

	Texture* Device::CreateTexture(const TextureDesc& desc, const TextureData* initialData)
	{
		Texture* texture = new Texture(*this);

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

		// Move all of the resource creation into Texture?
		if (!texture->Init(resource))
		{
			delete texture;
			return nullptr;
		}
		return texture;
	}

	Buffer* Device::CreateBuffer(const BufferDesc& desc)
	{
		Buffer* buffer = new Buffer(*this);

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

		// Move all of the resource creation into Buffer?
		if (!buffer->Init(resource))
		{
			delete buffer;
			return nullptr;
		}
		return buffer;
	}

	vkr::Render::Texture* Device::LoadTexture(const std::filesystem::path& filepath)
	{
		TextureLoader* loader = nullptr;
		auto loaderSearch = m_TextureLoaderByExtension.find(filepath.extension());
		if (loaderSearch != m_TextureLoaderByExtension.end())
		{
			loader = loaderSearch->second;
		}
		else
		{
			return nullptr;
		}

		TextureDesc textureDesc = {};
		TextureData textureData = {};
		if (!loader->LoadTexture(textureDesc, textureData, filepath))
		{
			return nullptr;
		}

		return CreateTexture(textureDesc, &textureData);
	}

	void Device::InitTextureLoaders()
	{
		TextureLoader* ddsLoader = new TextureLoader_DDS;
		m_TextureLoaderByExtension[".dds"] = ddsLoader;

		TextureLoader* pngLoader = new TextureLoader_PNG;
		m_TextureLoaderByExtension[".png"] = pngLoader;

		TextureLoader* tgaLoader = new TextureLoader_TGA;
		m_TextureLoaderByExtension[".tga"] = pngLoader;
	}

	void Device::InitCommandQueues()
	{
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		m_Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_CommandQueue[CONTEXT_TYPE_GRAPHICS]));
		m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_CommandQueueFence[CONTEXT_TYPE_GRAPHICS]));
		m_CommandQueueFenceValue[CONTEXT_TYPE_GRAPHICS] = 0;

		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		m_Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_CommandQueue[CONTEXT_TYPE_COMPUTE]));
		m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_CommandQueueFence[CONTEXT_TYPE_COMPUTE]));
		m_CommandQueueFenceValue[CONTEXT_TYPE_COMPUTE] = 0;

		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		m_Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_CommandQueue[CONTEXT_TYPE_COPY]));
		m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_CommandQueueFence[CONTEXT_TYPE_COPY]));
		m_CommandQueueFenceValue[CONTEXT_TYPE_COPY] = 0;
	}

	ID3D12Device* Device::GetD3DDevice() const
	{
		return m_Device.Get();
	}

	IDXGIFactory2* Device::GetDXGIFactory() const
	{
		return m_Factory.Get();
	}

	IDXGIAdapter1* Device::GetDXGIAdapter() const
	{
		return m_Adapter.Get();
	}

	ID3D12CommandQueue* Device::GetCommandQueue(ContextType contextType) const
	{
		return m_CommandQueue[contextType].Get();
	}

}