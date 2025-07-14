#include "commandqueue.h"
#include "device.h"
#include "commandlist.h"

namespace vkr::Render
{
	CommandQueue::CommandQueue(ContextType type)
		: m_FenceResource(MakeUnique<FenceResource>())
		, m_Type(type)
	{
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		switch (type)
		{
		default:
		case CONTEXT_TYPE_GRAPHICS:
			cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			break;
		case CONTEXT_TYPE_COMPUTE:
			cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			break;
		case CONTEXT_TYPE_COPY:
			cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
			break;

		}
		GetDevice()->GetD3DDevice()->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_CommandQueue));
	}

	CommandQueue::~CommandQueue()
	{
		m_FenceResource->Wait(m_FenceResource->GetLastValue());
	}

	Fence CommandQueue::Signal()
	{
		Fence event;
		event.m_Value = m_FenceResource->Increment();
		event.m_FenceResource = m_FenceResource.get();

		m_CommandQueue->Signal(m_FenceResource->GetFence(), event.m_Value);

		return event;
	}

	void CommandQueue::InsertWait(const Fence& event)
	{
		// No need to wait for work on the same queue.
		if (event.m_FenceResource == m_FenceResource.get())
			return;

		m_CommandQueue->Wait(event.m_FenceResource->GetFence(), event.m_Value);
	}

	bool CommandQueue::Wait(bool block)
	{
		return m_FenceResource->Wait(m_FenceResource->GetLastValue());
	}

	Fence CommandQueue::Submit(const Ref<CommandList>& commandList)
	{
		return Submit(1, &commandList);
	}

	Fence CommandQueue::Submit(uint32_t numCommandLists, const Ref<CommandList>* commandLists)
	{
		std::vector<ID3D12CommandList*> cmdLists;
		for (uint32_t i = 0; i < numCommandLists; ++i)
		{
			if (commandLists[i])
				cmdLists.push_back(commandLists[i]->GetD3DCommandList());
		}

		if (cmdLists.size() > 0)
		{
			m_CommandQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());
			return Signal();
		}
		else
		{
			return Fence();
		}
	}

	Fence CommandQueue::GetNextFence() const
	{
		Fence fence;
		fence.m_Value = m_FenceResource->GetNextValue();
		fence.m_FenceResource = m_FenceResource.get();
		return fence;
	}

	ID3D12CommandQueue* CommandQueue::GetD3DCommandQueue() const
	{
		return m_CommandQueue.Get();
	}
}