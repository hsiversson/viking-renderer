#include "context.h"

#include "buffer.h"
#include "pipelinestate.h"
#include "rootsignature.h"
#include "device.h"
#include "commandlist.h"
#include "commandqueue.h"
#include "d3dconvert.h"

namespace vkr::Render
{
	Context::Context(Device& device, ContextType type)
		: DeviceObject(device)
		, m_CurrentD3DCommandList(nullptr)
		, m_CurrentD3DCommandList7(nullptr)
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
		m_CurrentD3DCommandList->QueryInterface(IID_PPV_ARGS(&m_CurrentD3DCommandList7));
		m_CommandList->Open();
	}

	void Context::End()
	{
		// insert potential auto transitions/barriers
		m_CommandList->Close();
		m_CommandListsToSubmit.push_back(m_CommandList);
		if (m_CurrentD3DCommandList7)
			m_CurrentD3DCommandList7->Release();
		m_CurrentD3DCommandList7 = nullptr;
		m_CurrentD3DCommandList = nullptr;
		m_CommandList = nullptr;
	}

	Event Context::Flush()
	{
		m_LastFlushEvent = m_Device.GetCommandQueue(m_Type)->Submit(m_CommandListsToSubmit.size(), m_CommandListsToSubmit.data());

		CommandListPool::PendingCommandLists pending;
		pending.m_CommandLists.insert(pending.m_CommandLists.end(), m_CommandListsToSubmit.begin(), m_CommandListsToSubmit.end());
		pending.m_Event = m_LastFlushEvent;
		m_Device.GetCommandListPool(m_Type)->ReturnCommandList(pending);

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
		// TODO: defer barriers to group them better?

		std::vector<D3D12_TEXTURE_BARRIER> barriers;
		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const TextureBarrierDesc& barrierDesc = barrierDescs[i];
			ResourceStateTracking& stateTracking = barrierDesc.m_Texture->GetStateTracking();

			D3D12_TEXTURE_BARRIER barrier = {};
			barrier.AccessAfter = D3DConvertResourceStateAccess(barrierDesc.m_TargetAccess);
			barrier.AccessBefore = D3DConvertResourceStateAccess(stateTracking.m_CurrentAccess);
			barrier.SyncAfter = D3DConvertResourceStateSync(barrierDesc.m_TargetSync);
			barrier.SyncBefore = D3DConvertResourceStateSync(stateTracking.m_CurrentSync);
			barrier.LayoutAfter = D3DConvertResourceStateLayout(barrierDesc.m_TargetLayout);
			barrier.LayoutBefore = D3DConvertResourceStateLayout(stateTracking.m_CurrentLayout);
			barrier.pResource = barrierDesc.m_Texture->GetD3DResource();

			barrier.Subresources.IndexOrFirstMipLevel = 0xffffffff;
			barrier.Subresources.NumMipLevels = 0;

			barriers.push_back(barrier);
		}

		if (!barriers.empty())
		{
			D3D12_BARRIER_GROUP barrierGroup = {};
			barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
			barrierGroup.pTextureBarriers = barriers.data();
			barrierGroup.NumBarriers = barriers.size();

			m_CurrentD3DCommandList7->Barrier(1, &barrierGroup);
		}
	}

	void Context::TextureBarrier(const TextureBarrierDesc& barrierDesc)
	{
		TextureBarrier(1, &barrierDesc);
	}

	void Context::BufferBarrier(uint32_t numBarriers, const BufferBarrierDesc* barrierDescs)
	{
		// TODO: defer barriers to group them better?

		std::vector<D3D12_BUFFER_BARRIER> barriers;
		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const BufferBarrierDesc& barrierDesc = barrierDescs[i];
			ResourceStateTracking& stateTracking = barrierDesc.m_Buffer->GetStateTracking();

			D3D12_BUFFER_BARRIER barrier = {};
			barrier.AccessAfter = D3DConvertResourceStateAccess(barrierDesc.m_TargetAccess);
			barrier.AccessBefore = D3DConvertResourceStateAccess(stateTracking.m_CurrentAccess);
			barrier.SyncAfter = D3DConvertResourceStateSync(barrierDesc.m_TargetSync);
			barrier.SyncBefore = D3DConvertResourceStateSync(stateTracking.m_CurrentSync);
			barrier.pResource = barrierDesc.m_Buffer->GetD3DResource();

			barriers.push_back(barrier);
		}

		if (!barriers.empty())
		{
			D3D12_BARRIER_GROUP barrierGroup = {};
			barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
			barrierGroup.pBufferBarriers = barriers.data();
			barrierGroup.NumBarriers = barriers.size();

			m_CurrentD3DCommandList7->Barrier(1, &barrierGroup);
		}
	}

	void Context::BufferBarrier(const BufferBarrierDesc& barrierDesc)
	{
		BufferBarrier(1, &barrierDesc);
	}

	void Context::GlobalBarrier(uint32_t numBarriers, const GlobalBarrierDesc* barrierDescs)
	{
		// TODO: defer barriers to group them better?

		std::vector<D3D12_GLOBAL_BARRIER> barriers;
		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GlobalBarrierDesc& barrierDesc = barrierDescs[i];

			D3D12_GLOBAL_BARRIER barrier = {};
			barrier.AccessAfter = D3DConvertResourceStateAccess(barrierDesc.m_TargetAccess);
			barrier.AccessBefore = D3DConvertResourceStateAccess(barrierDesc.m_SourceAccess);
			barrier.SyncAfter = D3DConvertResourceStateSync(barrierDesc.m_TargetSync);
			barrier.SyncBefore = D3DConvertResourceStateSync(barrierDesc.m_SourceSync);

			barriers.push_back(barrier);
		}

		if (!barriers.empty())
		{
			D3D12_BARRIER_GROUP barrierGroup = {};
			barrierGroup.Type = D3D12_BARRIER_TYPE_GLOBAL;
			barrierGroup.pGlobalBarriers = barriers.data();
			barrierGroup.NumBarriers = barriers.size();

			m_CurrentD3DCommandList7->Barrier(1, &barrierGroup);
		}
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