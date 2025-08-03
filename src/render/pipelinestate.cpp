#include "pipelinestate.h"
#include "rootsignature.h"
#include "device.h"
#include "buffer.h"
#include "d3dconvert.h"

namespace vkr::Render
{
	PipelineState::PipelineState()
		: m_RootSignature(nullptr)
	{

	}

	PipelineState::~PipelineState()
	{

	}

	bool PipelineState::Init(const PipelineStateDesc& desc, const Ref<RootSignature> rootSignature)
	{
		m_RootSignature = rootSignature;
		m_MetaData.m_Type = desc.m_Type;

		switch (desc.m_Type)
		{
		case PIPELINE_STATE_TYPE_DEFAULT:
		default:
			return InitDefault(desc, rootSignature);
		case PIPELINE_STATE_TYPE_COMPUTE:
			return InitCompute(desc, rootSignature);
		case PIPELINE_STATE_TYPE_RAYTRACING:
			return InitRaytracing(desc, rootSignature);
		}

		return true;
	}

	ID3D12PipelineState* PipelineState::GetD3DPipelineState() const
	{
		return m_PipelineState.Get();
	}

	ID3D12StateObject* PipelineState::GetD3DStateObject() const
	{
		return m_StateObject.Get();
	}

	const D3D12_DISPATCH_RAYS_DESC& PipelineState::GetD3DDispatchRaysDesc() const
	{
		return m_RaytracingDispatchDesc;
	}

	const Ref<RootSignature>& PipelineState::GetRootSignature() const
	{
		return m_RootSignature;
	}

	const PipelineStateMetaData& PipelineState::GetMetaData() const
	{
		return m_MetaData;
	}

	PipelineStateType PipelineState::GetType() const
	{
		return m_MetaData.m_Type;
	}

	bool PipelineState::InitDefault(const PipelineStateDesc& desc, Ref<RootSignature> rootSignature)
	{
		ID3D12Device* device = GetDevice()->GetD3DDevice();
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsDesc = {};

		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
		for (auto& attrib : desc.Default.m_VertexLayout.GetAttributes())
		{
			D3D12_INPUT_ELEMENT_DESC element = {};
			element.Format = D3DConvertFormat(attrib.m_Format);
			element.InputSlot = attrib.m_BufferSlot;
			element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			element.SemanticIndex = attrib.m_Index;
			element.SemanticName = VertexAttribute::GetTypeSemantic(attrib.m_Type);
			element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			inputElements.push_back(element);
		}
		graphicsDesc.InputLayout.NumElements = static_cast<UINT>(inputElements.size());
		graphicsDesc.InputLayout.pInputElementDescs = inputElements.data();

		graphicsDesc.pRootSignature = rootSignature->GetD3DRootSignature();
		graphicsDesc.VS = { desc.Default.m_VertexShader->GetByteCode(), desc.Default.m_VertexShader->GetByteCodeSize() };
		if (desc.Default.m_PixelShader)
		{
			graphicsDesc.PS = { desc.Default.m_PixelShader->GetByteCode(), desc.Default.m_PixelShader->GetByteCodeSize() };
		}

		{
			const RasterizerState& rasterState = desc.Default.m_RasterizerState;
			graphicsDesc.RasterizerState = {};
			graphicsDesc.RasterizerState.AntialiasedLineEnable = rasterState.m_AntialiasedLine;
			graphicsDesc.RasterizerState.FillMode = rasterState.m_Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
			graphicsDesc.RasterizerState.FrontCounterClockwise = rasterState.m_FrontIsCounterClockwise;
			switch (rasterState.m_CullMode)
			{
			case FACE_CULL_MODE_NONE:
				graphicsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
				break;
			default:
			case FACE_CULL_MODE_BACK:
				graphicsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
				break;
			case FACE_CULL_MODE_FRONT:
				graphicsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
				break;
			}
		}
		{
			const BlendState& blendState = desc.Default.m_BlendState;
			graphicsDesc.BlendState = {};
			graphicsDesc.BlendState.AlphaToCoverageEnable = false;
			graphicsDesc.BlendState.IndependentBlendEnable = blendState.RTBlends.size() > 1;
			for (size_t i = 0; i < blendState.RTBlends.size(); i++)
			{
				graphicsDesc.BlendState.RenderTarget[i].BlendEnable = blendState.RTBlends[i].m_Enabled;
				graphicsDesc.BlendState.RenderTarget[i].BlendOp = D3DConvertBlendOp(blendState.RTBlends[i].m_Operation);
				graphicsDesc.BlendState.RenderTarget[i].SrcBlend = D3DConvertBlendArg(blendState.RTBlends[i].m_SrcBlend);
				graphicsDesc.BlendState.RenderTarget[i].DestBlend = D3DConvertBlendArg(blendState.RTBlends[i].m_DstBlend);
				graphicsDesc.BlendState.RenderTarget[i].BlendOpAlpha = D3DConvertBlendOp(blendState.RTBlends[i].m_AlphaOperation);
				graphicsDesc.BlendState.RenderTarget[i].SrcBlendAlpha = D3DConvertBlendArg(blendState.RTBlends[i].m_SrcBlendAlpha);
				graphicsDesc.BlendState.RenderTarget[i].DestBlendAlpha = D3DConvertBlendArg(blendState.RTBlends[i].m_DstBlendAlpha);
				graphicsDesc.BlendState.RenderTarget[i].LogicOpEnable = false; //Do we want to use this?
				graphicsDesc.BlendState.RenderTarget[i].RenderTargetWriteMask = blendState.RTBlends[i].m_WriteMask;
			}

		}
		{
			const DepthStencilState& depthStencilState = desc.Default.m_DepthStencilState;
			graphicsDesc.DepthStencilState = {};
			graphicsDesc.DepthStencilState.DepthEnable = depthStencilState.m_Enabled;
			graphicsDesc.DepthStencilState.DepthWriteMask = depthStencilState.m_WriteDepth ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
			graphicsDesc.DepthStencilState.DepthFunc = D3DConvertComparisonFunc(depthStencilState.m_ComparisonFunc);
			graphicsDesc.DepthStencilState.StencilEnable = false;
			graphicsDesc.DSVFormat = D3DConvertFormat(desc.Default.m_DepthStencilState.m_DSFormat);
		}
		graphicsDesc.SampleMask = UINT_MAX;
		graphicsDesc.PrimitiveTopologyType = D3DConvertPrimitiveType(desc.Default.m_PrimitiveType);

		graphicsDesc.NumRenderTargets = 0;
		for (uint32_t i = 0; i < MAX_NUM_RENDER_TARGETS; i++)
		{
			if (desc.Default.m_RenderTargetState.m_Formats[i] != FORMAT_UNKNOWN)
			{
				graphicsDesc.RTVFormats[i] = D3DConvertFormat(desc.Default.m_RenderTargetState.m_Formats[i]);
				++graphicsDesc.NumRenderTargets;

			}
		}
		graphicsDesc.SampleDesc.Count = 1;

		if (FAILED(device->CreateGraphicsPipelineState(&graphicsDesc, IID_PPV_ARGS(&m_PipelineState))))
		{
			return false;
		}
		return true;
	}

	bool PipelineState::InitCompute(const PipelineStateDesc& desc, Ref<RootSignature> rootSignature)
	{
		ID3D12Device* device = GetDevice()->GetD3DDevice();

		D3D12_COMPUTE_PIPELINE_STATE_DESC ComputeDesc = {};
		ComputeDesc.pRootSignature = rootSignature->GetD3DRootSignature();
		ComputeDesc.CS = { desc.Compute.m_ComputeShader->GetByteCode(), desc.Compute.m_ComputeShader->GetByteCodeSize() };
		ComputeDesc.NodeMask = 1;
		ComputeDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		if (FAILED(device->CreateComputePipelineState(&ComputeDesc, IID_PPV_ARGS(&m_PipelineState))))
		{
			return false;
		}

		m_MetaData.Compute.m_NumThreads = desc.Compute.m_ComputeShader->GetNumThreads();
		return true;
	}

	bool PipelineState::InitRaytracing(const PipelineStateDesc& desc, Ref<RootSignature> rootSignature)
	{
		ID3D12Device10* device = GetDevice()->GetD3DDevice10();

		const RaytracingPipelineStateDesc& raytracingDesc = desc.Raytracing;

		std::vector<D3D12_STATE_SUBOBJECT> subObjects;

		D3D12_GLOBAL_ROOT_SIGNATURE globalRootSignature = {};
		globalRootSignature.pGlobalRootSignature = rootSignature->GetD3DRootSignature();
		subObjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &globalRootSignature });

		D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
		shaderConfig.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
		shaderConfig.MaxPayloadSizeInBytes = 32;
		subObjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig });

		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
		pipelineConfig.MaxTraceRecursionDepth = 1;
		subObjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig });

		const uint32_t numHitGroups = raytracingDesc.m_NumHitGroups;

		std::vector<std::vector<D3D12_EXPORT_DESC>> exports;
		exports.resize(numHitGroups + 1); // 1 array per hit group plus 1 for ray gen and miss

		assert(!raytracingDesc.m_RayGenerationIdentifier.empty());
		assert(!raytracingDesc.m_MissIdentifier.empty());

		const std::wstring rayGenerationIdentifier = UTF8ToUTF16(raytracingDesc.m_RayGenerationIdentifier);
		const std::wstring missIdentifier = UTF8ToUTF16(raytracingDesc.m_MissIdentifier);

		exports[0].push_back({ rayGenerationIdentifier.c_str(), nullptr, D3D12_EXPORT_FLAG_NONE});
		exports[0].push_back({ missIdentifier.c_str(), nullptr, D3D12_EXPORT_FLAG_NONE });

		D3D12_DXIL_LIBRARY_DESC rayGenDxilDesc = {};
		rayGenDxilDesc.DXILLibrary.BytecodeLength = raytracingDesc.m_Shader->GetByteCodeSize();
		rayGenDxilDesc.DXILLibrary.pShaderBytecode = raytracingDesc.m_Shader->GetByteCode();
		rayGenDxilDesc.pExports = exports[0].data();
		rayGenDxilDesc.NumExports = exports[0].size();
		subObjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &rayGenDxilDesc });

		std::vector<D3D12_HIT_GROUP_DESC> hitGroups;
		hitGroups.reserve(numHitGroups);

		std::vector<D3D12_DXIL_LIBRARY_DESC> dxilLibraries;
		dxilLibraries.reserve(numHitGroups);

		std::vector<std::wstring> stringStorage;
		stringStorage.reserve(numHitGroups * 2);

		std::vector<std::wstring> hitGroupIdentifiers;
		hitGroupIdentifiers.reserve(numHitGroups);

		for (uint32_t i = 0; i < numHitGroups; ++i)
		{
			const RaytracingHitGroupDesc& hitGroupDesc = raytracingDesc.m_HitGroups[i];
			hitGroupIdentifiers.push_back(UTF8ToUTF16(hitGroupDesc.m_Identifier));
			const std::wstring& hitGroupIdentifier = hitGroupIdentifiers.back();
			stringStorage.push_back(UTF8ToUTF16(hitGroupDesc.m_ClosestHitIdentifier));
			const std::wstring& closestHitIdentifier = stringStorage.back();
			stringStorage.push_back(UTF8ToUTF16(hitGroupDesc.m_AnyHitIdentifier));
			const std::wstring& anyHitIdentifier = stringStorage.back();

			if (!closestHitIdentifier.empty())
			{
				exports[i + 1].push_back({ closestHitIdentifier.c_str(), nullptr, D3D12_EXPORT_FLAG_NONE});
			}
			if (!anyHitIdentifier.empty())
			{
				exports[i + 1].push_back({ anyHitIdentifier.c_str(), nullptr, D3D12_EXPORT_FLAG_NONE});
			}

			D3D12_DXIL_LIBRARY_DESC hitGroupDxilDesc = {};
			hitGroupDxilDesc.DXILLibrary.BytecodeLength = hitGroupDesc.m_Shader->GetByteCodeSize();
			hitGroupDxilDesc.DXILLibrary.pShaderBytecode = hitGroupDesc.m_Shader->GetByteCode();
			hitGroupDxilDesc.pExports = exports[i + 1].data();
			hitGroupDxilDesc.NumExports = exports[i + 1].size();
			dxilLibraries.push_back(hitGroupDxilDesc);
			subObjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &dxilLibraries[i] });

			D3D12_HIT_GROUP_DESC d3dHitGroupDesc = {};
			d3dHitGroupDesc.HitGroupExport = hitGroupIdentifier.c_str();
			d3dHitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
			if (!closestHitIdentifier.empty())
			{
				d3dHitGroupDesc.ClosestHitShaderImport = closestHitIdentifier.c_str();
			}
			if (!anyHitIdentifier.empty())
			{
				d3dHitGroupDesc.AnyHitShaderImport = anyHitIdentifier.c_str();
			}

			hitGroups.push_back(d3dHitGroupDesc);
			subObjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroups[i] });
		}

		D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
		stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
		stateObjectDesc.NumSubobjects = subObjects.size();
		stateObjectDesc.pSubobjects = subObjects.data();

		if (FAILED(device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&m_StateObject))))
		{
			return false;
		}

		// Create shader table
		ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		m_StateObject.As(&stateObjectProperties);

		constexpr uint32_t tableSize = Align(uint32_t(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES), uint32_t(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
		constexpr uint32_t recordSize = Align(uint32_t(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES), uint32_t(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT));

		const uint32_t rayGenerationOffset = 0;
		const uint32_t missOffset = 1 * tableSize;
		const uint32_t hitGroupsOffset = 2 * tableSize;
		const uint32_t hitGroupsSize = Align(numHitGroups * recordSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

		BufferDesc bufferDesc = {};
		bufferDesc.m_ElementCount = 2 * tableSize + hitGroupsSize;
		bufferDesc.m_ElementSize = 1;
		bufferDesc.m_Name = "Raytracing Shader Table";

		std::vector<uint8_t> data;
		data.resize(bufferDesc.m_ElementCount);

		const void* rayGenerationId = stateObjectProperties->GetShaderIdentifier(rayGenerationIdentifier.c_str());
		memcpy(data.data() + rayGenerationOffset, rayGenerationId, recordSize);

		const void* missId = stateObjectProperties->GetShaderIdentifier(missIdentifier.c_str());
		memcpy(data.data() + missOffset, missId, recordSize);

		for (uint32_t i = 0; i < numHitGroups; ++i)
		{
			const RaytracingHitGroupDesc& hitGroupDesc = raytracingDesc.m_HitGroups[i];
			const void* hitGroupId = stateObjectProperties->GetShaderIdentifier(hitGroupIdentifiers[i].c_str());

			uint32_t offset = hitGroupsOffset + i * recordSize;
			memcpy(data.data() + offset, hitGroupId, recordSize);
		}

		m_RaytracingShaderTable = GetDevice()->CreateBuffer(bufferDesc, data.size(), data.data());

		const D3D12_GPU_VIRTUAL_ADDRESS address = m_RaytracingShaderTable->GetD3DResource()->GetGPUVirtualAddress();
		m_RaytracingDispatchDesc = {};
		m_RaytracingDispatchDesc.RayGenerationShaderRecord.StartAddress = address + rayGenerationOffset;
		m_RaytracingDispatchDesc.RayGenerationShaderRecord.SizeInBytes = recordSize;
		m_RaytracingDispatchDesc.MissShaderTable.StartAddress = address + missOffset;
		m_RaytracingDispatchDesc.MissShaderTable.SizeInBytes = recordSize;
		m_RaytracingDispatchDesc.MissShaderTable.StrideInBytes = recordSize;
		m_RaytracingDispatchDesc.HitGroupTable.StartAddress = address + hitGroupsOffset;
		m_RaytracingDispatchDesc.HitGroupTable.SizeInBytes = hitGroupsSize;
		m_RaytracingDispatchDesc.HitGroupTable.StrideInBytes = recordSize;

		return true;
	}
}