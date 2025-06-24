#include "pipelinestate.h"
#include "rootsignature.h"
#include "device.h"
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
		ID3D12Device* device = GetDevice().GetD3DDevice();

		HRESULT hr;
		switch (desc.m_Type)
		{
		case PIPELINE_STATE_TYPE_COMPUTE:
		{
			D3D12_COMPUTE_PIPELINE_STATE_DESC ComputeDesc;
			ComputeDesc.pRootSignature = rootSignature->GetD3DRootSignature();
			ComputeDesc.CS = { desc.Compute.m_ComputeShader->GetByteCode(), desc.Compute.m_ComputeShader->GetByteCodeSize() };
			ComputeDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			hr = device->CreateComputePipelineState(&ComputeDesc, IID_PPV_ARGS(&m_PipelineState));
			if (FAILED(hr))
			{
				return false;
			}

			m_MetaData.Compute.m_NumThreads = desc.Compute.m_ComputeShader->GetNumThreads();
		}
		break;
		case PIPELINE_STATE_TYPE_DEFAULT:
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsDesc = {};

			std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
			for (auto& attrib : desc.Default.m_VertexLayout.m_Attributes)
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
			graphicsDesc.PS = { desc.Default.m_PixelShader->GetByteCode(), desc.Default.m_PixelShader->GetByteCodeSize() };

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
				for (int i = 0; i < blendState.RTBlends.size(); i++)
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

			hr = device->CreateGraphicsPipelineState(&graphicsDesc, IID_PPV_ARGS(&m_PipelineState));
			if (FAILED(hr))
			{
				_com_error err(hr);
				OutputDebugString(err.ErrorMessage());
				return false;
			}
		}
		break;
		}

		m_RootSignature = rootSignature;

		m_MetaData.m_Type = desc.m_Type;
		return true;
	}

	ID3D12PipelineState* PipelineState::GetD3DPipelineState() const
	{
		return m_PipelineState.Get();
	}

	const Ref<RootSignature>& PipelineState::GetRootSignature() const
	{
		return m_RootSignature;
	}

	const PipelineStateMetaData& PipelineState::GetMetaData() const
	{
		return m_MetaData;
	}
}