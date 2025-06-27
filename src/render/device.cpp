#include "device.h"
#include "descriptorheap.h"
#include "shadercompiler.h"
#include "rootsignature.h"
#include "textureloader_dds.h"
#include "textureloader_png.h"
#include "textureloader_tga.h"
#include "commandlist.h"
#include "commandqueue.h"
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
		m_Device.As(&m_Device10);

		m_ShaderCompiler = MakeUnique<ShaderCompiler>();

		InitDescriptorHeaps();
		InitRootSignatures();
		InitTextureLoaders();
		InitCommandQueues();
		InitTempBuffer();
		return true;
	}

	void Device::InitRootSignatures()
	{
		for (int i = 0; i < PipelineStateType::PIPELINE_STATE_TYPE_COUNT; i++)
		{
			auto Signature = MakeRef<RootSignature>();
			// For now just consider one unique constant buffer?
			Signature->Init({ PipelineStateType(i), 1 });
			m_RootSignatures[i] = Signature;
		}
	}

	Ref<Context> Device::CreateContext(ContextType contextType)
	{
		Ref<Context> context = MakeRef<Context>(contextType);
		return context;
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
		if (!texture->Init(desc,initialData))
		{
			return nullptr;
		}
		return texture;
	}

	Ref<Buffer> Device::CreateBuffer(const BufferDesc& desc, uint32_t initialDataSize, const void* initialData)
	{
		Ref<Buffer> buffer = MakeRef<Buffer>();
		if (!buffer->Init(desc, initialDataSize, initialData))
			return nullptr;
		
		return buffer;
	}

	Ref<Texture> Device::LoadTexture(const std::filesystem::path& filepath)
	{
		TextureLoader* loader = nullptr;
		auto loaderSearch = m_TextureLoaderByExtension.find(filepath.extension());
		if (loaderSearch != m_TextureLoaderByExtension.end())
		{
			loader = loaderSearch->second.get();
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
		m_TextureLoaderByExtension[".dds"] = MakeUnique<TextureLoader_DDS>();
		m_TextureLoaderByExtension[".png"] = MakeUnique<TextureLoader_PNG>();
		m_TextureLoaderByExtension[".tga"] = MakeUnique<TextureLoader_TGA>();
	}

	void Device::InitCommandQueues()
	{
		m_CommandQueue[CONTEXT_TYPE_GRAPHICS] = MakeRef<CommandQueue>(CONTEXT_TYPE_GRAPHICS);
		m_CommandListPool[CONTEXT_TYPE_GRAPHICS] = MakeRef<CommandListPool>(CONTEXT_TYPE_GRAPHICS);
		m_Contexts[CONTEXT_TYPE_GRAPHICS] = MakeRef<Context>(CONTEXT_TYPE_GRAPHICS);

		m_CommandQueue[CONTEXT_TYPE_COMPUTE] = MakeRef<CommandQueue>(CONTEXT_TYPE_COMPUTE);
		m_CommandListPool[CONTEXT_TYPE_COMPUTE] = MakeRef<CommandListPool>(CONTEXT_TYPE_COMPUTE);
		m_Contexts[CONTEXT_TYPE_COMPUTE] = MakeRef<Context>(CONTEXT_TYPE_COMPUTE);

		m_CommandQueue[CONTEXT_TYPE_COPY] = MakeRef<CommandQueue>(CONTEXT_TYPE_COPY);
		m_CommandListPool[CONTEXT_TYPE_COPY] = MakeRef<CommandListPool>(CONTEXT_TYPE_COPY);
		m_Contexts[CONTEXT_TYPE_COPY] = MakeRef<Context>(CONTEXT_TYPE_COPY);
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

	DescriptorHeap* Device::GetDescriptorHeap(DescriptorHeapType type) const
	{
		return m_DescriptorHeaps[type].get();
	}

	Ref<TextureView> Device::CreateTextureView(const TextureViewDesc& desc, const Ref<Texture>& resource)
	{
		Ref<TextureView> textureView = MakeRef<TextureView>();
		if (!textureView->Init(desc, resource))
		{
			return nullptr;
		}
		return textureView;
	}

	Ref<RenderTargetView> Device::CreateRenderTargetView(const RenderTargetViewDesc& desc, const Ref<Texture>& resource)
	{
		Ref<RenderTargetView> rtv = MakeRef<RenderTargetView>();
		if (!rtv->Init(desc, resource))
		{
			return nullptr;
		}
		return rtv;
	}

	Ref<DepthStencilView> Device::CreateDepthStencilView(const DepthStencilViewDesc& desc, const Ref<Texture>& resource)
	{
		Ref<DepthStencilView> dsv = MakeRef<DepthStencilView>();
		if (!dsv->Init(desc, resource))
		{
			return nullptr;
		}
		return dsv;
	}

	Ref<BufferView> Device::CreateBufferView(const BufferViewDesc& desc, const Ref<Buffer>& resource)
	{
		Ref<BufferView> bufferView = MakeRef<BufferView>();
		if (!bufferView->Init(desc, resource))
		{
			return nullptr;
		}
		return bufferView;
	}

	TempBuffer Device::GetTempBuffer(uint32_t byteSize, uint32_t initialDataSize, const void* initialData)
	{
		auto Align = [](uint32_t value, uint32_t alignment)
		{
			return ((value + alignment - 1) / alignment) * alignment;
		};

		// 256 is mainly for constant buffers though. We should align differently based on buffer usage
		const uint32_t size = Align(byteSize, 256);
		assert("TempBuffer is full" && (m_TempBufferCurrentOffset + size) <= m_TempBuffer->GetDesc().ByteSize());

		TempBuffer outTempBuffer;
		outTempBuffer.m_Buffer = m_TempBuffer.get();		
		outTempBuffer.m_Offset = m_TempBufferCurrentOffset.fetch_add(size);

		if (initialData)
		{
			outTempBuffer.m_Buffer->UploadData(outTempBuffer.m_Offset, initialDataSize, initialData);
		}
		return outTempBuffer;
	}

	Ref<Buffer> Device::CreateTLAS(uint32_t numRtInstanceDescs, RtInstanceDesc* rtInstanceDescs)
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = buildDesc.Inputs;
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;

		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
		for (uint32_t i = 0; i < numRtInstanceDescs; ++i)
		{
			const RtInstanceDesc& rtInstanceDesc = rtInstanceDescs[i];
			D3D12_RAYTRACING_INSTANCE_DESC desc = {};
			desc.AccelerationStructure = rtInstanceDesc.m_BLAS->GetD3DResource()->GetGPUVirtualAddress();
			desc.InstanceID = rtInstanceDesc.m_InstanceId;
			desc.InstanceMask = 0xff;
			// TODO: the other instance desc params
			instanceDescs.push_back(desc);
		}

		const uint32_t bufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDescs.size();
		TempBuffer instanceDescsBuffer = GetTempBuffer(bufferSize, bufferSize, instanceDescs.data());
		inputs.InstanceDescs = instanceDescsBuffer.m_Buffer->GetD3DResource()->GetGPUVirtualAddress() + instanceDescsBuffer.m_Offset;
		inputs.NumDescs = instanceDescs.size();

		return CreateRaytracingAccelerationStructure(buildDesc);
	}

	Ref<Buffer> Device::CreateBLAS(uint32_t numRtGeometryDescs, RtGeometryDesc* rtGeometryDescs)
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = buildDesc.Inputs;
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
		for (uint32_t i = 0; i < numRtGeometryDescs; ++i)
		{
			const RtGeometryDesc& rtGeometryDesc = rtGeometryDescs[i];
			D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
			desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

			const BufferDesc& indexBufferDesc = rtGeometryDesc.m_IndexBuffer->GetDesc();
			desc.Triangles.IndexBuffer = rtGeometryDesc.m_IndexBuffer->GetD3DResource()->GetGPUVirtualAddress();
			desc.Triangles.IndexFormat = D3DConvertFormat(indexBufferDesc.m_Format);
			desc.Triangles.IndexCount = indexBufferDesc.m_ElementCount;

			const BufferDesc& vertexBufferDesc = rtGeometryDesc.m_VertexBuffer->GetDesc();
			desc.Triangles.VertexBuffer.StartAddress = rtGeometryDesc.m_VertexBuffer->GetD3DResource()->GetGPUVirtualAddress();
			desc.Triangles.VertexBuffer.StrideInBytes = vertexBufferDesc.m_ElementSize;
			desc.Triangles.VertexFormat = D3DConvertFormat(vertexBufferDesc.m_Format);
			desc.Triangles.IndexCount = vertexBufferDesc.m_ElementCount;

			geometryDescs.push_back(desc);
		}

		inputs.pGeometryDescs = geometryDescs.data();
		inputs.NumDescs = geometryDescs.size();

		return CreateRaytracingAccelerationStructure(buildDesc);
	}

	void Device::InitDescriptorHeaps()
	{
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE_SHADER_RESOURCE] = MakeUnique<DescriptorHeap>(DESCRIPTOR_HEAP_TYPE_SHADER_RESOURCE, 1000000);
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE_SAMPLER] = MakeUnique<DescriptorHeap>(DESCRIPTOR_HEAP_TYPE_SAMPLER, 128);
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE_RENDER_TARGET] = MakeUnique<DescriptorHeap>(DESCRIPTOR_HEAP_TYPE_RENDER_TARGET, 512);
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE_DEPTH_STENCIL] = MakeUnique<DescriptorHeap>(DESCRIPTOR_HEAP_TYPE_DEPTH_STENCIL, 16);
	}

	Ref<Buffer> Device::CreateRaytracingAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& buildDesc)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		m_Device10->GetRaytracingAccelerationStructurePrebuildInfo(&buildDesc.Inputs, &prebuildInfo);

		TempBuffer scratchBuffer = GetTempBuffer(prebuildInfo.ScratchDataSizeInBytes);
		buildDesc.ScratchAccelerationStructureData = scratchBuffer.m_Buffer->GetD3DResource()->GetGPUVirtualAddress() + scratchBuffer.m_Offset;

		BufferDesc outBufferDesc = {};
		outBufferDesc.m_ElementCount = prebuildInfo.ResultDataMaxSizeInBytes;

		Ref<Buffer> outBuffer = CreateBuffer(outBufferDesc);
		buildDesc.DestAccelerationStructureData = outBuffer->GetD3DResource()->GetGPUVirtualAddress();

		Ref<CommandList> cmdList = m_RaytracingBuildPool->GetCommandList();
		ID3D12GraphicsCommandList* d3dCmdList = cmdList->GetD3DCommandList();
		ID3D12GraphicsCommandList4* d3dCmdList4 = nullptr;
		d3dCmdList->QueryInterface(IID_PPV_ARGS(&d3dCmdList4));

		d3dCmdList4->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

		Event event = m_RaytracingBuildQueue->Submit(cmdList);
		m_RaytracingBuildPool->ReturnCommandList(cmdList, event);

		// set event on outBuffer to have it track its build status
		outBuffer->SetGpuPending(event);

		d3dCmdList4->Release();
		return outBuffer;
	}
	vkr::Ref<vkr::Render::Context> Device::GetContext(ContextType contextType) const
	{
		return m_Contexts[contextType];
	}

	void Device::GarbageCollect()
	{
		m_TempBufferCurrentOffset = 0;
	}

	void Device::InitTempBuffer()
	{
		BufferDesc tempBufferDesc = {};
		tempBufferDesc.m_CpuWritable = true;
		tempBufferDesc.m_Writable = false;
		tempBufferDesc.m_ElementCount = 8 * 1024 * 1024; // 8MB should be enough for now
		tempBufferDesc.m_ElementSize = 1;
		tempBufferDesc.m_Format = FORMAT_UNKNOWN;
		m_TempBuffer = CreateBuffer(tempBufferDesc);
		m_TempBufferCurrentOffset = 0;
	}
}