#include "commandlist.h"
#include "device.h"

namespace vkr::Render
{
	CommandSignature::CommandSignature(CommandSignatureType type)
		: m_Type(type)
	{
		D3D12_COMMAND_SIGNATURE_DESC desc = {};
		D3D12_INDIRECT_ARGUMENT_DESC argDesc;
		switch (type)
		{
			case COMMAND_SIGNAUTRE_DRAW:
			{
				argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

				desc.NumArgumentDescs = 1;
				desc.pArgumentDescs = &argDesc;
				desc.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS);
				break;
			}
			case COMMAND_SIGNAUTRE_DRAW_INDEXED:
			{
				argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

				desc.NumArgumentDescs = 1;
				desc.pArgumentDescs = &argDesc;
				desc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
				break;
			}
			case COMMAND_SIGNAUTRE_DISPATCH:
			{
				argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

				desc.NumArgumentDescs = 1;
				desc.pArgumentDescs = &argDesc;
				desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
				break;
			}
			default:
				VKR_CHECK_NO_ENTRY();
		}


		ID3D12Device* device = GetDevice()->GetD3DDevice();
		if (FAILED(device->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&m_CommandSignature))))
		{
			VKR_CHECK_NO_ENTRY();
		}
	}

	ID3D12CommandSignature* CommandSignature::GetD3DCommandSignature() const
	{
		return m_CommandSignature.Get();
	}

	uint64_t CommandSignature::GetRequiredArgumentBufferSize() const
	{
		switch (m_Type)
		{
		case COMMAND_SIGNAUTRE_DRAW:
		{
			return sizeof(D3D12_DRAW_ARGUMENTS);
		}
		case COMMAND_SIGNAUTRE_DRAW_INDEXED:
		{
			return sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
		}
		case COMMAND_SIGNAUTRE_DISPATCH:
		{
			return sizeof(D3D12_DISPATCH_ARGUMENTS);
		}
		default:
			VKR_CHECK_NO_ENTRY();
			return 0;
		}
	}

	CommandSignatureType CommandSignature::GetType() const
	{
		return m_Type;
	}

	CommandList::CommandList(ContextType type)
		: m_Type(type)
	{
		D3D12_COMMAND_LIST_TYPE cmdListType;
		switch (type)
		{
		default:
		case CONTEXT_TYPE_GRAPHICS:
			cmdListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
			break;
		case CONTEXT_TYPE_COMPUTE:
			cmdListType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			break;
		case CONTEXT_TYPE_COPY:
			cmdListType = D3D12_COMMAND_LIST_TYPE_COPY;
			break;
		}

		ID3D12Device* device = GetDevice()->GetD3DDevice();
		device->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(&m_Allocator));
		device->CreateCommandList(0, cmdListType, m_Allocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList));
		m_CommandList->Close();
	}

	void CommandList::Open()
	{
		m_Allocator->Reset();
		m_CommandList->Reset(m_Allocator.Get(), nullptr);
	}

	void CommandList::Close()
	{
		m_CommandList->Close();
	}

	ContextType CommandList::GetType() const
	{
		return m_Type;
	}

	ID3D12GraphicsCommandList* CommandList::GetD3DCommandList() const
	{
		return m_CommandList.Get();
	}

	CommandListPool::CommandListPool(ContextType type)
		: m_Type(type)
	{

	}

	CommandListPool::~CommandListPool()
	{

	}

	Ref<CommandList> CommandListPool::GetCommandList()
	{
		CheckPendingCommandLists();

		Ref<CommandList> cmdList;
		std::unique_lock<std::mutex> lock(m_FreeListMutex);
		if (m_FreeCommandLists.size() > 0)
		{
			cmdList = m_FreeCommandLists.back();
			m_FreeCommandLists.pop_back();
		}
		else
		{
			cmdList = MakeRef<CommandList>(m_Type);
		}
		return cmdList;
	}

	void CommandListPool::ReturnCommandList(Ref<CommandList> commandList, Fence event)
	{
		PendingCommandLists pending;
		pending.m_CommandLists.push_back(commandList);
		pending.m_Event = event;
		ReturnCommandList(pending);
	}

	void CommandListPool::ReturnCommandList(const PendingCommandLists& pendingCommandLists)
	{
		{
			std::unique_lock<std::mutex> lock(m_PendingListMutex);
			m_PendingCommandLists.push(pendingCommandLists);
		}
		CheckPendingCommandLists();
	}

	ContextType CommandListPool::GetType() const
	{
		return m_Type;
	}

	void CommandListPool::CheckPendingCommandLists()
	{
		std::vector<Ref<CommandList>> freeCommandLists;
		{
			std::unique_lock<std::mutex> lock(m_PendingListMutex);
			while (!m_PendingCommandLists.empty())
			{
				const PendingCommandLists& pending = m_PendingCommandLists.front();
				if (!pending.m_Event.IsPending())
				{
					freeCommandLists.insert(freeCommandLists.end(), pending.m_CommandLists.begin(), pending.m_CommandLists.end());
					m_PendingCommandLists.pop();
				}
				else
				{
					break;
				}
			}
		}

		std::unique_lock<std::mutex> lock(m_FreeListMutex);
		m_FreeCommandLists.insert(m_FreeCommandLists.end(), freeCommandLists.begin(), freeCommandLists.end());
	}
}