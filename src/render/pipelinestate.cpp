#include "pipelinestate.h"
#include "rootsignature.h"

namespace vkr::Render
{

	PipelineState::PipelineState()
	{

	}

	PipelineState::~PipelineState()
	{

	}

	bool PipelineState::Init(const PipelineStateDesc& desc, RootSignature* rootSignature)
	{

		return true;
	}

	ID3D12PipelineState* PipelineState::GetD3D12PipelineState() const
	{
		return m_PipelineState.Get();
	}

}