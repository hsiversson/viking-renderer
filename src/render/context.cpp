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

	void Context::BindRootConstantBuffers(std::vector<Buffer*> buffers)
	{
		NewState.m_RootCB = buffers;
		m_StateUpdate = true;
	}

	void Context::UpdateState()
	{
		if (m_StateUpdate)
		{
			if (CurrentState.m_VertexBuffers != NewState.m_VertexBuffers)
			{
				std::vector<D3D12_VERTEX_BUFFER_VIEW> bufferviews;
				int idx = 0;
				for (auto buffer : NewState.m_VertexBuffers)
				{
					D3D12_VERTEX_BUFFER_VIEW view;
					view.BufferLocation = buffer->GetD3DResource()->GetGPUVirtualAddress();
					view.SizeInBytes = buffer->m_BufferDesc.ElementCount * buffer->m_BufferDesc.ElementSize;
					view.StrideInBytes = buffer->m_BufferDesc.ElementSize;
					bufferviews.push_back(view);
				}
				m_CurrentD3DCommandList->IASetVertexBuffers(0, bufferviews.size(), bufferviews.data());
			}
			if (CurrentState.m_IndexBuffer != NewState.m_IndexBuffer)
			{
				D3D12_INDEX_BUFFER_VIEW view;
				view.BufferLocation = NewState.m_IndexBuffer->GetD3DResource()->GetGPUVirtualAddress();
				view.SizeInBytes = NewState.m_IndexBuffer->m_BufferDesc.ElementSize * NewState.m_IndexBuffer->m_BufferDesc.ElementCount;
				view.Format = NewState.m_IndexBuffer->m_BufferDesc.ElementSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
				m_CurrentD3DCommandList->IASetIndexBuffer(&view);
			}
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

			if (m_RenderTargetUpdate)
			{
				//Assemble final list of render targets removing null ones
				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE*> FinalRT;
				for (auto Descriptor : NewState.m_RenderTargets)
				{
					FinalRT.push_back(&Descriptor->GetHandle());
				}
				
				m_CurrentD3DCommandList->OMSetRenderTargets(FinalRT.size(),*FinalRT.data(),false,&NewState.m_DepthStencil->GetHandle());
				m_RenderTargetUpdate = false;
			}
			

			CurrentState = NewState;
			m_StateUpdate = false;
		}
	}

	ContextType Context::GetType() const
	{
		return m_Type;
	}

	void Context::BindRenderTargets(std::vector<Ref<ResourceDescriptor>> rtdescriptors)
	{
		if (NewState.m_RenderTargets != rtdescriptors)
		{
			NewState.m_RenderTargets = rtdescriptors;
			m_StateUpdate = true;
			m_RenderTargetUpdate = true;
		}
	}

	void Context::SetDepthStencil(Ref<ResourceDescriptor> dsdescriptor)
	{
		if (NewState.m_DepthStencil != dsdescriptor)
		{
			NewState.m_DepthStencil = dsdescriptor;
			m_StateUpdate = true;
			m_RenderTargetUpdate = true;
		}
	}

	void Context::BindVertexBuffers(std::vector<Ref<Buffer>> vertexbuffers)
	{
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
}