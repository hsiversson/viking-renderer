#include "context.h"

#include "buffer.h"
#include "pipelinestate.h"
#include "rootsignature.h"
#include "device.h"
#include "commandlist.h"
#include "commandqueue.h"

namespace vkr::Render
{
	Context::Context(Device& device, ContextType type)
		: DeviceObject(device)
		, m_CurrentD3DCommandList(nullptr)
		, m_Type(type)
	{
	}

	Context::~Context()
	{
	}

	void Context::Init(ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator)
	{
		//m_CommandList = commandList;
		//m_CommandAllocator = commandAllocator;
	}

	void Context::Begin()
	{
		m_CommandList = m_Device.GetCommandListPool(m_Type)->GetCommandList();
		m_CurrentD3DCommandList = m_CommandList->GetD3DCommandList();
		m_CommandList->Open();
	}

	void Context::End()
	{
		// insert potential auto transitions/barriers
		m_CommandList->Close();
		m_CommandListsToSubmit.push_back(m_CommandList);
		m_CurrentD3DCommandList = nullptr;
		m_CommandList = nullptr;
	}

	Event Context::Flush()
	{
		m_LastFlushEvent = m_Device.GetCommandQueue(m_Type)->Submit(m_CommandListsToSubmit.size(), m_CommandListsToSubmit.data());
		m_Device.GetCommandListPool(m_Type)->ReturnCommandList(m_CommandListsToSubmit.size(), m_CommandListsToSubmit.data());
		m_CommandListsToSubmit.clear();

		return m_LastFlushEvent;
	}

	void Context::Dispatch(const Vector3u& Groups)
	{
		UpdateState();
		m_CurrentD3DCommandList->Dispatch(Groups.x, Groups.y, Groups.z);
	}

	void Context::DispatchThreads(PipelineState* pipelineState, const Vector3u& threads)
	{
		BindPSO(pipelineState);
		DispatchThreads(threads);
	}

	void Context::DispatchThreads(const Vector3u& threads)
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

	void Context::BindPSO(PipelineState* pipelineState)
	{
		NewState.m_PipelineState = pipelineState;
		NewState.m_RootSignature = pipelineState->GetRootSignature().get();
		m_StateUpdate = true;
	}

	void Context::BindRootConstantBuffers(std::vector<Buffer*> buffers)
	{
		NewState.m_RootCB = buffers;
		m_StateUpdate = true;
	}

	void Context::TextureBarrier(uint32_t numBarriers, const TextureBarrierDesc* barrierDescs)
	{
		// do we defer barriers to group them better?
	}

	void Context::TextureBarrier(const TextureBarrierDesc& barrierDesc)
	{
		TextureBarrier(1, &barrierDesc);
	}

	void Context::BufferBarrier(uint32_t numBarriers, const BufferBarrierDesc* barrierDescs)
	{
		// do we defer barriers to group them better?
	}

	void Context::BufferBarrier(const BufferBarrierDesc& barrierDesc)
	{
		BufferBarrier(1, &barrierDesc);
	}

	void Context::GlobalBarrier(uint32_t numBarriers, const GlobalBarrierDesc* barrierDescs)
	{
		// do we defer barriers to group them better?
	}

	void Context::GlobalBarrier(const GlobalBarrierDesc& barrierDesc)
	{
		GlobalBarrier(1, &barrierDesc);
	}

	void Context::UpdateState()
	{
		if (m_StateUpdate)
		{
			if (CurrentState.m_PipelineState != NewState.m_PipelineState)
			{
				m_CurrentD3DCommandList->SetPipelineState(NewState.m_PipelineState->GetD3DPipelineState());
			}
			if (CurrentState.m_RootSignature != NewState.m_RootSignature)
			{
				m_CurrentD3DCommandList->SetComputeRootSignature(NewState.m_RootSignature->GetD3DRootSignature());
			}

			for (int i = 0; i < NewState.m_RootCB.size(); i++)
			{
				auto buffer = NewState.m_RootCB[i];
				if ((CurrentState.m_RootCB.size() <= i) || (CurrentState.m_RootCB[i] != buffer))
				{
					//Aditional buffer bound or different buffer bound at a previously bound slot
					m_CurrentD3DCommandList->SetComputeRootConstantBufferView(i, buffer->GetD3DResource()->GetGPUVirtualAddress());
				}
			}

			CurrentState = NewState;
			m_StateUpdate = false;
		}
	}

	ContextType Context::GetType() const
	{
		return m_Type;
	}
}