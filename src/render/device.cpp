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

#include "utils/commandline.h"
#include "utils/hash.h"

#include <algorithm>

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 615; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = "./"; }

namespace vkr::Render
{
	static const D3D12_HEAP_PROPERTIES DefHeapProps{ D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
	static const D3D12_HEAP_PROPERTIES UploadHeapProps{ D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };

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
		m_Device.As(&m_Device5);

		m_ShaderCompiler = MakeUnique<ShaderCompiler>();

		InitDescriptorHeaps();
		InitRootSignatures();
		InitTextureLoaders();
		InitCommandQueues();
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

		Ref<Context> context = MakeRef<Context>(contextType);
		context->Init(commandList, commandAllocator);
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
		texture->m_TextureDesc = desc;

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
		TextureDesc.Flags = desc.bWriteable ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
		TextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		//If we want the texture to be used as UAV assume the initial state then will be UAV access
		HRESULT hr = m_Device->CreateCommittedResource(&DefHeapProps, D3D12_HEAP_FLAG_NONE, &TextureDesc, desc.bWriteable ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource));
		if (FAILED(hr))
		{
			return nullptr;
		}

		// Move all of the resource creation into Texture?
		if (!texture->InitWithResource(resource))
		{
			return nullptr;
		}
		return texture;
	}

	Ref<Buffer> Device::CreateBuffer(const BufferDesc& desc)
	{
		Ref<Buffer> buffer = MakeRef<Buffer>();

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
		if (!buffer->InitWithResource(resource))
		{
			return nullptr;
		}
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

		m_CommandQueue[CONTEXT_TYPE_COMPUTE] = MakeRef<CommandQueue>(CONTEXT_TYPE_COMPUTE);
		m_CommandListPool[CONTEXT_TYPE_COMPUTE] = MakeRef<CommandListPool>(CONTEXT_TYPE_COMPUTE);

		m_CommandQueue[CONTEXT_TYPE_COPY] = MakeRef<CommandQueue>(CONTEXT_TYPE_COPY);
		m_CommandListPool[CONTEXT_TYPE_COPY] = MakeRef<CommandListPool>(CONTEXT_TYPE_COPY);

		m_RaytracingBuildQueue = MakeRef<CommandQueue>(CONTEXT_TYPE_COMPUTE);
		m_RaytracingBuildPool = MakeRef<CommandListPool>(CONTEXT_TYPE_COMPUTE);
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

	const Ref<CommandQueue>& Device::GetCommandQueue(ContextType contextType) const
	{
		return m_CommandQueue[contextType];
	}

	const Ref<CommandListPool>& Device::GetCommandListPool(ContextType contextType) const
	{
		return m_CommandListPool[contextType];
	}

	Ref<ResourceDescriptor> Device::GetOrCreateDescriptor(Texture* tex, const ResourceDescriptorDesc& desc)
	{
		const uint8_t* buffer = reinterpret_cast<const uint8_t*>(&desc);
		uint64_t hashvalue = vkr::hash_fnv64(buffer, buffer + sizeof(ResourceDescriptorDesc));
		auto descriptor = tex->GetDescriptor(hashvalue);
		if (!descriptor)
		{
			switch (desc.Type)
			{
				case ResourceDescriptorType::UAV:
				{
					descriptor = m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
					D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
					uavDesc.Format = tex->m_TextureDesc.Format; 
					if (tex->m_TextureDesc.Dimension == 1)
					{
						uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
						uavDesc.Texture1D.MipSlice = desc.TextureDesc.Mip;
					}
					else if (tex->m_TextureDesc.Dimension == 2)
					{
						uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
						uavDesc.Texture2D.PlaneSlice = 0;
						uavDesc.Texture2D.MipSlice = desc.TextureDesc.Mip;
					}
					m_Device->CreateUnorderedAccessView(tex->GetD3DResource(), nullptr, &uavDesc, descriptor->GetHandle());
				}
				break;
				case ResourceDescriptorType::SRV:
				{
					descriptor = m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
					srvDesc.Format = tex->m_TextureDesc.Format;
					srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					if (tex->m_TextureDesc.Dimension == 1)
					{
						srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
						srvDesc.Texture1D.MipLevels = desc.TextureDesc.Mip;
					}
					else if (tex->m_TextureDesc.Dimension == 2)
					{
						srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
						srvDesc.Texture2D.PlaneSlice = 0;
						srvDesc.Texture2D.MostDetailedMip = 0;
						srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
					}
					m_Device->CreateShaderResourceView(tex->GetD3DResource(), &srvDesc, descriptor->GetHandle());
				}
				break;
			}

			tex->AddDescriptor(hashvalue, descriptor);
		}
		return descriptor;
	}

	Ref<ResourceDescriptor> Device::GetOrCreateDescriptor(Buffer* buf, const ResourceDescriptorDesc& desc)
	{
		const uint8_t* buffer = reinterpret_cast<const uint8_t*>(&desc);
		uint64_t hashvalue = vkr::hash_fnv64(buffer, buffer + sizeof(ResourceDescriptorDesc));
		auto descriptor = buf->GetDescriptor(hashvalue);
		if (!descriptor)
		{
			switch (desc.Type)
			{
			case ResourceDescriptorType::UAV:
			{
				descriptor = m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = DXGI_FORMAT_UNKNOWN;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				uavDesc.Buffer.FirstElement = desc.BufferDesc.First;
				uavDesc.Buffer.NumElements = desc.BufferDesc.Last - desc.BufferDesc.First;
				uavDesc.Buffer.StructureByteStride = desc.BufferDesc.ElementSize;
				uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
				m_Device->CreateUnorderedAccessView(buf->GetD3DResource(), nullptr, &uavDesc, descriptor->GetHandle());
			}
			break;
			case ResourceDescriptorType::SRV:
			{
				descriptor = m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = DXGI_FORMAT_UNKNOWN;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Buffer.FirstElement = desc.BufferDesc.First;
				srvDesc.Buffer.NumElements = desc.BufferDesc.Last - desc.BufferDesc.First;
				srvDesc.Buffer.StructureByteStride = desc.BufferDesc.ElementSize;
				srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				m_Device->CreateShaderResourceView(buf->GetD3DResource(), &srvDesc, descriptor->GetHandle());
			}
			break;
			}

			buf->AddDescriptor(hashvalue, descriptor);
		}
		return descriptor;
	}

	TempBuffer Device::GetTempBuffer(uint32_t byteSize, uint32_t initialDataSize, const void* initialData)
	{
		// TODO
		assert(false);
		return TempBuffer();
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
			desc.Triangles.IndexCount = indexBufferDesc.Size;

			const BufferDesc& vertexBufferDesc = rtGeometryDesc.m_VertexBuffer->GetDesc();
			desc.Triangles.VertexBuffer.StartAddress = rtGeometryDesc.m_VertexBuffer->GetD3DResource()->GetGPUVirtualAddress();
			desc.Triangles.VertexBuffer.StrideInBytes = vertexBufferDesc.m_ElementSize;
			desc.Triangles.VertexFormat = D3DConvertFormat(vertexBufferDesc.m_Format);
			desc.Triangles.IndexCount = vertexBufferDesc.Size;

			geometryDescs.push_back(desc);
		}

		inputs.pGeometryDescs = geometryDescs.data();
		inputs.NumDescs = geometryDescs.size();

		return CreateRaytracingAccelerationStructure(buildDesc);
	}

	void Device::InitDescriptorHeaps()
	{
		ID3D12DescriptorHeap* d3dheap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc;
		HeapDesc.NumDescriptors = 1,000,000;
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		HeapDesc.NodeMask = 0;

		HRESULT hr = m_Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&d3dheap));
		if (FAILED(hr))
		{

		}

		UINT descriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		auto heap = new DescriptorHeap;
		heap->Init(d3dheap, 1000000, descriptorSize);
		m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = heap;

		HeapDesc.NumDescriptors = 100;
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		hr = m_Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&d3dheap));
		if (FAILED(hr))
		{

		}

		descriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		heap = new DescriptorHeap;
		heap->Init(d3dheap, 100, descriptorSize);
		m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = heap;

		HeapDesc.NumDescriptors = 500;
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		hr = m_Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&d3dheap));
		if (FAILED(hr))
		{

		}

		descriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		heap = new DescriptorHeap;
		heap->Init(d3dheap, 500, descriptorSize);
		m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = heap;

		HeapDesc.NumDescriptors = 16;
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		hr = m_Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&d3dheap));
		if (FAILED(hr))
		{

		}

		descriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		heap = new DescriptorHeap;
		heap->Init(d3dheap, 16, descriptorSize);
		m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = heap;
	}

	Ref<Buffer> Device::CreateRaytracingAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& buildDesc)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		m_Device5->GetRaytracingAccelerationStructurePrebuildInfo(&buildDesc.Inputs, &prebuildInfo);

		TempBuffer scratchBuffer = GetTempBuffer(prebuildInfo.ScratchDataSizeInBytes);
		buildDesc.ScratchAccelerationStructureData = scratchBuffer.m_Buffer->GetD3DResource()->GetGPUVirtualAddress() + scratchBuffer.m_Offset;

		BufferDesc outBufferDesc = {};
		outBufferDesc.Size = prebuildInfo.ResultDataMaxSizeInBytes;

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
}