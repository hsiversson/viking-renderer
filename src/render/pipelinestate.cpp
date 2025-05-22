#include "pipelinestate.h"
#include "rootsignature.h"
#include "device.h"

namespace vkr::Render
{
	PipelineState::PipelineState(Device& device)
		: DeviceObject(device)
		, m_RootSignature(nullptr)
	{

	}

	PipelineState::~PipelineState()
	{

	}

	bool PipelineState::Init(const PipelineStateDesc& desc, RootSignature* rootSignature)
	{
		ID3D12Device* device = m_Device.GetD3DDevice();

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
		}
		break;
		case PIPELINE_STATE_TYPE_DEFAULT:
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsDesc;
			GraphicsDesc.InputLayout = { desc.Default.m_VertexLayout.m_D3DElements.data(), (unsigned int)desc.Default.m_VertexLayout.m_D3DElements.size() };
			GraphicsDesc.pRootSignature = rootSignature->GetD3DRootSignature();
			GraphicsDesc.VS = {desc.Default.m_VertexShader->GetByteCode(), desc.Default.m_VertexShader->GetByteCodeSize()};
			GraphicsDesc.PS = { desc.Default.m_PixelShader->GetByteCode(), desc.Default.m_PixelShader->GetByteCodeSize() };
			GraphicsDesc.RasterizerState = desc.Default.m_RasterizerState.m_D3DRasterizerState;
			GraphicsDesc.BlendState = desc.Default.m_BlendState.m_D3DBlendDesc;
			GraphicsDesc.DepthStencilState = desc.Default.m_DepthStencilState.m_D3DDepthStencilState;
			GraphicsDesc.SampleMask = UINT_MAX;
			GraphicsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // DO WE WANT TO BE ABLE TO RENDER OTHER TOPOLOGIES?
			GraphicsDesc.NumRenderTargets = desc.Default.m_RenderTargetState.m_D3DRTFormats.size();
			for (int i = 0; i < GraphicsDesc.NumRenderTargets; i++)
			{
				GraphicsDesc.RTVFormats[0] = desc.Default.m_RenderTargetState.m_D3DRTFormats[i];
			}
			GraphicsDesc.SampleDesc.Count = 1;

			hr = device->CreateGraphicsPipelineState(&GraphicsDesc, IID_PPV_ARGS(&m_PipelineState));
			if (FAILED(hr))
			{
				return false;
			}
		}
		break;
		}

		m_RootSignature = rootSignature;
		return true;
	}

	ID3D12PipelineState* PipelineState::GetD3DPipelineState() const
	{
		return m_PipelineState.Get();
	}

	vkr::Render::RootSignature* PipelineState::GetRootSignature() const
	{
		return m_RootSignature;
	}

}