#include "device.h"

#include "commandlist.h"
#include "commandqueue.h"
#include "descriptorheap.h"
#include "rootsignature.h"
#include "shadercompiler.h"
#include "textureloader_dds.h"
#include "textureloader_png.h"
#include "textureloader_tga.h"

#include "d3dconvert.h"

#include "core/commandline.h"
#include "utils/hash.h"

#include <algorithm>

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 615; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = "./"; }

namespace vkr::Render
{
	Device* Device::g_Instance = nullptr;

	Device::Device()
	{
		assert(g_Instance == nullptr);
		g_Instance = this;
	}

	Device::~Device()
	{
		g_Instance = nullptr;

		for (uint32_t i = 0; i < CONTEXT_TYPE_COUNT; ++i)
		{
			m_RenderThreads[i]->Stop();
		}
		m_RenderResourceDestructionQueue->Flush();
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

		{
			ComPtr<IDXGIFactory6> factory6;
			m_Factory.As(&factory6);

			uint32_t selectedAdapterId = 0;
			ComPtr<IDXGIAdapter1> selectedAdapter;
			DXGI_ADAPTER_DESC1 selectedAdapterDesc = {};

			{
				ComPtr<IDXGIAdapter1> adapter;
				DXGI_ADAPTER_DESC1 adapterDesc = {};

				VKR_LOG(L"Available adapters:");
				VKR_LOG(L"[");
				for (uint32_t i = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter))); ++i)
				{
					adapter->GetDesc1(&adapterDesc);
					VKR_LOG(L"	[{}]: {}", i, adapterDesc.Description);

					if (adapterDesc.DedicatedVideoMemory > selectedAdapterDesc.DedicatedVideoMemory)
					{
						selectedAdapterId = i;
						selectedAdapter = adapter;
						selectedAdapterDesc = adapterDesc;
					}
				}
				VKR_LOG(L"]");
			}

			m_Adapter = selectedAdapter;
			VKR_LOG(L"Adapter selected: [{}]: {}", selectedAdapterId, selectedAdapterDesc.Description);
		}

		D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device));
		m_Device.As(&m_Device10);


		InitDescriptorHeaps();
		InitRootSignatures();
		InitTextureLoaders();
		InitCommandQueues();

		for (uint32_t i = 0; i < TEMP_BUFFER_USAGE_COUNT; ++i)
		{
			m_TempBufferAllocators[i] = MakeUnique<TempBufferAllocator>(TempBufferUsage(i), 1 * 1024 * 1024);
		}

		m_ShaderCompiler = MakeUnique<ShaderCompiler>();
		m_RenderResourceDestructionQueue = MakeUnique<RenderResourceDestructionQueue>();
		m_RenderResourceDestructionQueue->Start();
		return true;
	}

	void Device::BeginFrame()
	{
		for (uint32_t i = 0; i < TEMP_BUFFER_USAGE_COUNT; ++i)
		{
			m_TempBufferAllocators[i]->StartChunk();
		}
	}

	void Device::EndFrame()
	{
		for (uint32_t i = 0; i < TEMP_BUFFER_USAGE_COUNT; ++i)
		{
			m_TempBufferAllocators[i]->EndChunk(GetCommandQueue(CONTEXT_TYPE_PRESENT)->Signal());
		}

		// TODO: add end chunk to all temp buffers pending delete
		// TODO: garbage collect temp buffers pending delete
	}

	void Device::WaitForGpuIdle()
	{
		QueueGraphicsTask([]() {})->Wait();
	}

	Ref<SwapChain> Device::CreateSwapChain(void* windowHandle, const Vector2u& size)
	{
		Ref<SwapChain> swapChain = MakeRef<SwapChain>();
		if (!swapChain->Init(windowHandle, size))
		{
			return nullptr;
		}
		return swapChain;
	}

	Ref<Shader> Device::CreateShader(const std::filesystem::path& filepath, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel)
	{
		Ref<Shader> shader = MakeRef<Shader>();
		if (!m_ShaderCompiler->CompileFromFile(*shader, filepath, entryPoint, stage, shaderModel))
		{
			return nullptr;
		}
		return shader;
	}

	Ref<Shader> Device::CreateShaderFromString(const std::string& sourceCode, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel)
	{
		Ref<Shader> shader = MakeRef<Shader>();
		if (!m_ShaderCompiler->CompileFromMemory(*shader, sourceCode, entryPoint, stage, shaderModel))
		{
			return nullptr;
		}
		return shader;
	}

	Ref<Shader> Device::CreateShaderFromString(const std::wstring& sourceCode, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel)
	{
		return CreateShaderFromString(UTF16ToUTF8(sourceCode), entryPoint, stage, shaderModel);
	}

	Ref<PipelineState> Device::CreatePipelineState(const PipelineStateDesc& desc)
	{
		Ref<PipelineState> pipelineState = MakeRef<PipelineState>();
		if (!pipelineState->Init(desc, m_RootSignatures[desc.m_Type]))
		{
			return nullptr;
		}
		return pipelineState;
	}

	Ref<Texture> Device::CreateTexture(const TextureDesc& desc, const TextureData* initialData)
	{
		Ref<Texture> texture = MakeRef<Texture>();

		// Move all of the resource creation into Texture?
		if (!texture->Init(desc, initialData))
		{
			return nullptr;
		}
		return texture;
	}

	Ref<Texture> Device::LoadTexture(const std::filesystem::path& filepath)
	{
		if (Ref<Texture> tex = m_TextureCache.Get(filepath))
		{
			return tex;
		}

		TextureLoader* loader = nullptr;
		auto loaderSearch = m_TextureLoaderByExtension.find(filepath.extension());
		if (loaderSearch != m_TextureLoaderByExtension.end())
		{
			loader = loaderSearch->second.get();
		}
		else
		{
			assert(false && "invalid extension");
			return nullptr;
		}

		TextureDesc textureDesc = {};
		TextureData textureData = {};
		if (!loader->LoadTexture(textureDesc, textureData, filepath))
		{
			assert(false && "failed to load texture");
			return nullptr;
		}

		Ref<Texture> texture = CreateTexture(textureDesc, &textureData);
		m_TextureCache.Insert(filepath, texture);
		return texture;
	}

	Ref<TextureView> Device::CreateTextureView(const TextureViewDesc& desc, const Ref<Texture>& resource)
	{
		Ref<TextureView> textureView = std::static_pointer_cast<TextureView>(resource->GetDescriptor(desc));
		if (textureView)
			return textureView;

		textureView = MakeRef<TextureView>();
		if (!textureView->Init(desc, resource))
		{
			assert(false && "failed to create texture view");
			return nullptr;
		}
		return textureView;
	}

	Ref<RenderTargetView> Device::CreateRenderTargetView(const RenderTargetViewDesc& desc, const Ref<Texture>& resource)
	{
		Ref<RenderTargetView> rtv = std::static_pointer_cast<RenderTargetView>(resource->GetDescriptor(desc));
		if (rtv)
			return rtv;

		rtv = MakeRef<RenderTargetView>();
		if (!rtv->Init(desc, resource))
		{
			return nullptr;
		}
		return rtv;
	}

	Ref<DepthStencilView> Device::CreateDepthStencilView(const DepthStencilViewDesc& desc, const Ref<Texture>& resource)
	{
		Ref<DepthStencilView> dsv = std::static_pointer_cast<DepthStencilView>(resource->GetDescriptor(desc));
		if (dsv)
			return dsv;

		dsv = MakeRef<DepthStencilView>();
		if (!dsv->Init(desc, resource))
		{
			return nullptr;
		}
		return dsv;
	}

	Ref<Buffer> Device::CreateBuffer(const BufferDesc& desc, uint32_t initialDataSize, const void* initialData)
	{
		Ref<Buffer> buffer = MakeRef<Buffer>();
		if (!buffer->Init(desc, initialDataSize, initialData))
			return nullptr;

		return buffer;
	}

	Ref<BufferView> Device::CreateBufferView(const BufferViewDesc& desc, const Ref<Buffer>& resource)
	{
		Ref<BufferView> bufferView = std::static_pointer_cast<BufferView>(resource->GetDescriptor(desc));
		if (bufferView)
			return bufferView;

		bufferView = MakeRef<BufferView>();
		if (!bufferView->Init(desc, resource))
		{
			return nullptr;
		}
		return bufferView;
	}

	TempBuffer Device::GetTempBuffer(TempBufferUsage usage, uint32_t byteSize, uint32_t initialDataSize, const void* initialData)
	{
		const uint32_t size = Align(byteSize, usage == TEMP_BUFFER_USAGE_CONSTANTS ? 256 : 4);

		TempBuffer outTempBuffer;
		if (!m_TempBufferAllocators[usage]->Allocate(size, outTempBuffer))
		{
			uint32_t currentBufferSize = m_TempBufferAllocators[usage]->GetCapacity();
			uint32_t newSize = ((size - currentBufferSize) + currentBufferSize) * 2;

			m_TempBuffersPendingDelete.push_back(std::move(m_TempBufferAllocators[usage]));

			m_TempBufferAllocators[usage] = MakeUnique<TempBufferAllocator>(usage, newSize);
			if (!m_TempBufferAllocators[usage]->Allocate(size, outTempBuffer))
			{
				assert(false);
				return TempBuffer();
			}
		}

		if (initialData)
		{
			outTempBuffer.m_Buffer->UploadData(outTempBuffer.m_Offset, initialDataSize, initialData);
		}
		return outTempBuffer;
	}

	Ref<Buffer> Device::CreateTLAS(uint32_t numRtInstanceDescs, RaytracingInstanceDesc* rtInstanceDescs)
	{
		RaytracingAccelerationStructureBuildDesc buildDesc = {};
		buildDesc.m_Type = RaytracingAccelerationStructureBuildDesc::Type::TopLevel;
		buildDesc.m_InstanceDescs.insert(buildDesc.m_InstanceDescs.end(), rtInstanceDescs, rtInstanceDescs + numRtInstanceDescs);

		std::unique_lock<std::mutex> lock(m_RaytracingBuildQueueMutex);
		m_RaytracingBuildContext->Begin();

		Ref<Buffer> tlas = m_RaytracingBuildContext->BuildRaytracingAccelerationStructure(buildDesc);

		m_RaytracingBuildContext->End();
		Fence event = m_RaytracingBuildContext->Flush();
		tlas->SetGpuPending(event);
		return tlas;
	}

	Ref<Buffer> Device::CreateBLAS(uint32_t numRtGeometryDescs, RaytracingGeometryDesc* rtGeometryDescs)
	{
		RaytracingAccelerationStructureBuildDesc buildDesc = {};
		buildDesc.m_Type = RaytracingAccelerationStructureBuildDesc::Type::BottomLevel;
		buildDesc.m_GeometryDescs.insert(buildDesc.m_GeometryDescs.end(), rtGeometryDescs, rtGeometryDescs + numRtGeometryDescs);

		std::unique_lock<std::mutex> lock(m_RaytracingBuildQueueMutex);
		m_RaytracingBuildContext->Begin();

		Ref<Buffer> blas = m_RaytracingBuildContext->BuildRaytracingAccelerationStructure(buildDesc);

		m_RaytracingBuildContext->End();
		Fence event = m_RaytracingBuildContext->Flush();
		blas->SetGpuPending(event);
		return blas;
	}

	void Device::SetCurrentSwapChain(const Ref<SwapChain>& swapChain)
	{
		m_CurrentSwapChain = swapChain;
	}

	const Ref<SwapChain>& Device::GetCurrentSwapChain() const
	{
		return m_CurrentSwapChain;
	}

	ID3D12Device* Device::GetD3DDevice() const
	{
		return m_Device.Get();
	}

	ID3D12Device10* Device::GetD3DDevice10() const
	{
		return m_Device10.Get();
	}

	IDXGIFactory2* Device::GetDXGIFactory() const
	{
		return m_Factory.Get();
	}

	IDXGIAdapter1* Device::GetDXGIAdapter() const
	{
		return m_Adapter.Get();
	}

	const Ref<CommandQueue>& Device::GetCommandQueue(ContextType contextType) const
	{
		return m_CommandQueue[contextType];
	}

	const Ref<CommandListPool>& Device::GetCommandListPool(ContextType contextType) const
	{
		return m_CommandListPool[contextType];
	}

	Ref<Context> Device::GetContext(ContextType contextType) const
	{
		return m_Contexts[contextType];
	}

	RenderThread* Device::GetRenderThread(ContextType contextType) const
	{
		return m_RenderThreads[contextType].get();
	}

	DescriptorHeap* Device::GetDescriptorHeap(DescriptorHeapType type) const
	{
		return m_DescriptorHeaps[type].get();
	}

	void Device::FlushDeferredDestructionQueue()
	{
		m_RenderResourceDestructionQueue->Flush();
	}

	void Device::InitRootSignatures()
	{
		for (int i = 0; i < PipelineStateType::PIPELINE_STATE_TYPE_COUNT; i++)
		{
			RootSignatureDesc desc = {};
			desc.m_PipelineUsage = PipelineStateType(i);
			desc.m_NumLocalConstantBuffers = 4;

			m_RootSignatures[i] = MakeRef<RootSignature>();
			if (!m_RootSignatures[i]->Init(desc))
			{
				assert(false && "failed to init root signature.");
				return;
			}
		}
	}

	void Device::InitTextureLoaders()
	{
		m_TextureLoaderByExtension[".dds"] = MakeUnique<TextureLoader_DDS>();
		m_TextureLoaderByExtension[".png"] = MakeUnique<TextureLoader_PNG>();
		m_TextureLoaderByExtension[".tga"] = MakeUnique<TextureLoader_TGA>();
	}

	void Device::InitCommandQueues()
	{
		for (uint32_t i = 0; i < CONTEXT_TYPE_COUNT; ++i)
		{
			const ContextType contextType = ContextType(i);
			m_CommandQueue[contextType] = MakeRef<CommandQueue>(contextType);
			m_CommandListPool[contextType] = MakeRef<CommandListPool>(contextType);
			m_Contexts[contextType] = MakeRef<Context>(contextType, m_CommandQueue[contextType]);
			m_RenderThreads[contextType] = MakeUnique<RenderThread>(contextType);
			m_RenderThreads[contextType]->Start();
		}

		m_RaytracingBuildQueue = MakeRef<CommandQueue>(CONTEXT_TYPE_COMPUTE);
		m_RaytracingBuildContext = MakeRef<Context>(CONTEXT_TYPE_COMPUTE, m_RaytracingBuildQueue);
	}

	void Device::InitDescriptorHeaps()
	{
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE_SHADER_RESOURCE] = MakeUnique<DescriptorHeap>(DESCRIPTOR_HEAP_TYPE_SHADER_RESOURCE, 1000000);
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE_SAMPLER] = MakeUnique<DescriptorHeap>(DESCRIPTOR_HEAP_TYPE_SAMPLER, 128);
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE_RENDER_TARGET] = MakeUnique<DescriptorHeap>(DESCRIPTOR_HEAP_TYPE_RENDER_TARGET, 512);
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE_DEPTH_STENCIL] = MakeUnique<DescriptorHeap>(DESCRIPTOR_HEAP_TYPE_DEPTH_STENCIL, 16);
	}

	Ref<RenderTaskEvent> QueueRenderTask(ContextType type, RenderTaskFn task, RenderTaskFlags flags)
	{
		if (Device* device = GetDevice())
		{
			return device->GetRenderThread(type)->QueueTask(task, flags);
		}
		else
		{
			return nullptr;
		}
	}

	Ref<RenderTaskEvent> QueueGraphicsTask(RenderTaskFn task, RenderTaskFlags flags)
	{
		return QueueRenderTask(CONTEXT_TYPE_GRAPHICS, task, flags);
	}

	Ref<RenderTaskEvent> QueueComputeTask(RenderTaskFn task, RenderTaskFlags flags)
	{
		return QueueRenderTask(CONTEXT_TYPE_COMPUTE, task, flags);
	}

	Ref<RenderTaskEvent> QueueCopyTask(RenderTaskFn task, RenderTaskFlags flags)
	{
		return QueueRenderTask(CONTEXT_TYPE_COPY, task, flags);
	}

}