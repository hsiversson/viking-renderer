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
	Context::Context(ContextType type)
		: m_CurrentD3DCommandList(nullptr)
		, m_CurrentD3DCommandList7(nullptr)
		, m_Type(type)
	{
	}

	Context::~Context()
	{
	}

	void Context::Begin()
	{
		m_CommandList = GetDevice()->GetCommandListPool(m_Type)->GetCommandList();
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
		CurrentState = {};
	}

	Event Context::Flush()
	{
		Device* device = GetDevice();
		m_LastFlushEvent = device->GetCommandQueue(m_Type)->Submit(m_CommandListsToSubmit.size(), m_CommandListsToSubmit.data());

		CommandListPool::PendingCommandLists pending;
		pending.m_CommandLists.insert(pending.m_CommandLists.end(), m_CommandListsToSubmit.begin(), m_CommandListsToSubmit.end());
		pending.m_Event = m_LastFlushEvent;
		device->GetCommandListPool(m_Type)->ReturnCommandList(pending);

		m_CommandListsToSubmit.clear();

		return m_LastFlushEvent;
	}

	void Context::Dispatch(const Vector3u& Groups)
	{
		UpdateState();
		m_CurrentD3DCommandList->Dispatch(Groups.x, Groups.y, Groups.z);
	}

	void Context::DispatchThreads(Ref<PipelineState> pipelineState, const Vector3u& threads)
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

	void Context::BindPSO(Ref<PipelineState> pipelineState)
	{
		NewState.m_PipelineState = pipelineState;
		NewState.m_RootSignature = pipelineState->GetRootSignature().get();
		m_StateUpdate = true;
	}

	void Context::BindRootConstantBuffers(Buffer** buffers, size_t bufferCount, uint64_t* offsets)
	{
		NewState.m_RootCB = std::vector<Buffer*>(buffers, buffers+bufferCount);
		NewState.m_RootCBOffsets.clear();
		if (offsets)
		{
			NewState.m_RootCBOffsets = std::vector<uint64_t>(offsets, offsets + bufferCount);
		}
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
			stateTracking.m_CurrentAccess = barrierDesc.m_TargetAccess;
			stateTracking.m_CurrentLayout = barrierDesc.m_TargetLayout;
			stateTracking.m_CurrentSync = barrierDesc.m_TargetSync;
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
				ID3D12DescriptorHeap* descriptorHeaps[2] = 
				{ 
					GetDevice()->GetDescriptorHeap(DESCRIPTOR_HEAP_TYPE_SHADER_RESOURCE)->GetD3DDescriptorHeap(),
					GetDevice()->GetDescriptorHeap(DESCRIPTOR_HEAP_TYPE_SAMPLER)->GetD3DDescriptorHeap()
				};
				m_CurrentD3DCommandList->SetDescriptorHeaps(2, descriptorHeaps);

				if (NewState.m_PipelineState->GetMetaData().m_Type == PIPELINE_STATE_TYPE_COMPUTE)
				{
					m_CurrentD3DCommandList->SetComputeRootSignature(NewState.m_RootSignature->GetD3DRootSignature());
				}
				else if (NewState.m_PipelineState->GetMetaData().m_Type == PIPELINE_STATE_TYPE_DEFAULT)
				{
					m_CurrentD3DCommandList->SetGraphicsRootSignature(NewState.m_RootSignature->GetD3DRootSignature());
				}
			}

			if (CurrentState.m_VertexBuffers != NewState.m_VertexBuffers)
			{
				std::vector<D3D12_VERTEX_BUFFER_VIEW> bufferviews;
				int idx = 0;
				for (auto buffer : NewState.m_VertexBuffers)
				{
					D3D12_VERTEX_BUFFER_VIEW view;
					view.BufferLocation = buffer->GetD3DResource()->GetGPUVirtualAddress();
					view.SizeInBytes = buffer->GetDesc().m_ElementCount * buffer->GetDesc().m_ElementSize;
					view.StrideInBytes = buffer->GetDesc().m_ElementSize;
					bufferviews.push_back(view);
				}
				m_CurrentD3DCommandList->IASetVertexBuffers(0, bufferviews.size(), bufferviews.data());
			}
			if (CurrentState.m_IndexBuffer != NewState.m_IndexBuffer)
			{
				D3D12_INDEX_BUFFER_VIEW view;
				view.BufferLocation = NewState.m_IndexBuffer->GetD3DResource()->GetGPUVirtualAddress();
				view.SizeInBytes = NewState.m_IndexBuffer->GetDesc().m_ElementSize * NewState.m_IndexBuffer->GetDesc().m_ElementCount;
				view.Format = NewState.m_IndexBuffer->GetDesc().m_ElementSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
				m_CurrentD3DCommandList->IASetIndexBuffer(&view);
			}
			if (CurrentState.m_Topology != NewState.m_Topology)
			{
				m_CurrentD3DCommandList->IASetPrimitiveTopology(D3DConvertPrimitiveTopology(NewState.m_Topology));
			}

			for (int i = 0; i < NewState.m_RootCB.size(); i++)
			{
				auto buffer = NewState.m_RootCB[i];
				if ((CurrentState.m_RootCB.size() <= i) || (CurrentState.m_RootCB[i] != buffer))
				{
					bool bIsCompute = NewState.m_PipelineState->GetMetaData().m_Type == PIPELINE_STATE_TYPE_COMPUTE;
					D3D12_GPU_VIRTUAL_ADDRESS addr = buffer->GetD3DResource()->GetGPUVirtualAddress();
					if (!NewState.m_RootCBOffsets.empty())
						addr += NewState.m_RootCBOffsets[i];
					//Aditional buffer bound or different buffer bound at a previously bound slot
					if (bIsCompute)
						m_CurrentD3DCommandList->SetComputeRootConstantBufferView(i, addr);
					else
						m_CurrentD3DCommandList->SetGraphicsRootConstantBufferView(i, addr);
				}
			}

			if (m_RenderTargetUpdate)
			{
				//Assemble final list of render targets removing null ones
				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE*> FinalRT;
				for (auto Descriptor : NewState.m_RenderTargets)
				{
					FinalRT.push_back(&Descriptor->GetHandle());
				}
				
				m_CurrentD3DCommandList->OMSetRenderTargets(FinalRT.size(),FinalRT.size() ? *FinalRT.data() : nullptr, false, &NewState.m_DepthStencil->GetHandle());
				m_RenderTargetUpdate = false;
			}
			

			CurrentState = NewState;
			m_StateUpdate = false;
		}
	}

	void Context::ClearRenderTargets(Ref<RenderTargetView>* rtvs, size_t numRtvs)
	{
		const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		for (uint32_t i = 0; i < numRtvs; ++i)
		{
			m_CurrentD3DCommandList->ClearRenderTargetView(rtvs[i]->GetHandle(), clearColor, 0, nullptr);
		}
	}

	void Context::ClearDepthStencil(Ref<DepthStencilView> dsv, float clearValue)
	{
		m_CurrentD3DCommandList->ClearDepthStencilView(dsv->GetHandle(), D3D12_CLEAR_FLAG_DEPTH, clearValue, 0, 0, nullptr);
	}

	ContextType Context::GetType() const
	{
		return m_Type;
	}

	CommandList* Context::GetCommandList() const
	{
		return m_CommandList.get();
	}

	void Context::BindRenderTargets(Ref<RenderTargetView>* rtviews, size_t viewCount)
	{
		std::vector<Ref<RenderTargetView>> rtdescriptors(rtviews, rtviews + viewCount);
		if (NewState.m_RenderTargets != rtdescriptors)
		{
			NewState.m_RenderTargets = rtdescriptors;
			m_StateUpdate = true;
			m_RenderTargetUpdate = true;
		}
	}

	void Context::BindDepthStencil(Ref<DepthStencilView> dsview)
	{
		if (NewState.m_DepthStencil != dsview)
		{
			NewState.m_DepthStencil = dsview;
			m_StateUpdate = true;
			m_RenderTargetUpdate = true;
		}
	}

	void Context::BindVertexBuffers(Ref<Buffer>* buffers, size_t buffercount)
	{
		std::vector<Ref<Buffer>> vertexbuffers(buffers, buffers + buffercount);
		if (NewState.m_VertexBuffers != vertexbuffers)
		{
			NewState.m_VertexBuffers = vertexbuffers;
			m_StateUpdate = true;
		}
	}

	void Context::BindIndexBuffer(Ref<Buffer> indexbuffer)
	{
		if (NewState.m_IndexBuffer != indexbuffer)
		{
			NewState.m_IndexBuffer = indexbuffer;
			m_StateUpdate = true;
		}
	}

	void Context::Draw(uint32_t vertexCount, uint32_t startVertex)
	{
		DrawInstanced(vertexCount, 1, startVertex, 0);
	}

	void Context::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
	{
		UpdateState();
		m_CurrentD3DCommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
	}

	void Context::DrawIndexed(uint32_t indexCount, uint32_t startIndex, uint32_t startVertex)
	{
		DrawIndexedInstanced(indexCount, 1, startIndex, startVertex, 0);
	}

	void Context::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, uint32_t startVertex, uint32_t startInstance)
	{
		UpdateState();
		m_CurrentD3DCommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, startVertex, startInstance);
	}

	void Context::SetPrimitiveTopology(PrimitiveTopology topologyType)
	{
		if (NewState.m_Topology != topologyType)
		{
			NewState.m_Topology = topologyType;
			m_StateUpdate = true;
		}
	}

	void Context::SetViewport(uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height, float depthMin /*= 0.0f*/, float depthMax /*= 1.0f*/)
	{
		D3D12_VIEWPORT vp;
		vp.TopLeftX = offsetX;
		vp.TopLeftY = offsetY;
		vp.Width = width;
		vp.Height = height;
		vp.MinDepth = depthMin;
		vp.MaxDepth = depthMax;
		m_CurrentD3DCommandList->RSSetViewports(1, &vp);
	}

	void Context::SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
	{
		D3D12_RECT rect;
		rect.left = left;
		rect.top = top;
		rect.right = right;
		rect.bottom = bottom;
		m_CurrentD3DCommandList->RSSetScissorRects(1, &rect);
	}

}