#include "context.h"

#include "buffer.h"
#include "pipelinestate.h"
#include "rootsignature.h"

void vkr::Render::Context::Init(ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator)
{
	m_CommandList = commandList;
	m_CommandAllocator = commandAllocator;
}

vkr::Render::Context::Context(Device& device, ContextType type)
	: DeviceObject(device)
	, m_Type(type)
{

}

vkr::Render::Context::~Context()
{

}

void vkr::Render::Context::Dispatch(const Vector3u& Groups)
{
	UpdateState();
	m_CommandList->Dispatch(Groups.x, Groups.y, Groups.z);
}

void vkr::Render::Context::BindPSO(PipelineState* pipelineState)
{
	NewState.m_PipelineState = pipelineState;
	NewState.m_RootSignature = pipelineState->GetRootSignature();
	m_StateUpdate = true;
}

void vkr::Render::Context::BindRootConstantBuffers(std::vector<Buffer*> buffers)
{
	NewState.m_RootCB = buffers;
	m_StateUpdate = true;
}

void vkr::Render::Context::UpdateState()
{
	if (m_StateUpdate)
	{
		if (CurrentState.m_PipelineState != NewState.m_PipelineState)
		{
			m_CommandList->SetPipelineState(NewState.m_PipelineState->GetD3DPipelineState());
		}
		if (CurrentState.m_RootSignature != NewState.m_RootSignature)
		{
			m_CommandList->SetComputeRootSignature(NewState.m_RootSignature->GetD3DRootSignature());
		}

		for (int i = 0; i < NewState.m_RootCB.size(); i++)
		{
			auto buffer = NewState.m_RootCB[i];
			if ((CurrentState.m_RootCB.size() <= i) || (CurrentState.m_RootCB[i] != buffer))
			{
				//Aditional buffer bound or different buffer bound at a previously bound slot
				m_CommandList->SetComputeRootConstantBufferView(i, buffer->GetD3DResource()->GetGPUVirtualAddress());
			}
		}

		CurrentState = NewState;
		m_StateUpdate = false;
	}
}

vkr::Render::ContextType vkr::Render::Context::GetType() const
{
	return m_Type;
}

void vkr::Render::Context::DispatchThreads(PipelineState* pipelineState, const Vector3u& threads)
{
	BindPSO(pipelineState);
	DispatchThreads(threads);
}

void vkr::Render::Context::DispatchThreads(const Vector3u& threads)
{
	assert(NewState.m_PipelineState);
	const PipelineStateMetaData& metaData = NewState.m_PipelineState->GetMetaData();
	assert(metaData.m_Type == PIPELINE_STATE_TYPE_COMPUTE);

	Vector3u threadGroups;
	threadGroups.x = (threads.x + metaData.Compute.m_NumThreads.x - 1) / metaData.Compute.m_NumThreads.x;
	threadGroups.y = (threads.y + metaData.Compute.m_NumThreads.y - 1) / metaData.Compute.m_NumThreads.y;
	threadGroups.z = (threads.z + metaData.Compute.m_NumThreads.z - 1) / metaData.Compute.m_NumThreads.z;
	Dispatch(threadGroups);
}
