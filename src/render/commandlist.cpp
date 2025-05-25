#include "commandlist.h"
#include "device.h"

namespace vkr::Render
{
	CommandList::CommandList(Device& device, ContextType type)
		: DeviceObject(device)
		, m_Type(type)
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

		m_Device.GetD3DDevice()->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(&m_Allocator));
		m_Device.GetD3DDevice()->CreateCommandList(0, cmdListType, m_Allocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList));
		m_CommandList->Close();
	}

	void CommandList::Open()
	{
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

	CommandListPool::CommandListPool(Device& device, ContextType type)
		: DeviceObject(device)
		, m_Type(type)
	{

	}

	CommandListPool::~CommandListPool()
	{

	}

	Ref<CommandList> CommandListPool::GetCommandList()
	{
		Ref<CommandList> cmdList;
		std::unique_lock<std::mutex> lock(m_Mutex);
		if (m_FreeCommandLists.size() > 0)
		{
			cmdList = m_FreeCommandLists.back();
			m_FreeCommandLists.pop_back();
		}
		else
		{
			cmdList = MakeRef<CommandList>(m_Device, m_Type);
		}
		return cmdList;
	}

	void CommandListPool::ReturnCommandList(const Ref<CommandList>& commandList)
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_FreeCommandLists.push_back(commandList);

	}

	void CommandListPool::ReturnCommandList(uint32_t numCommandLists, const Ref<CommandList>* commandLists)
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_FreeCommandLists.reserve(m_FreeCommandLists.size() + numCommandLists);
		for (uint32_t i = 0; i < numCommandLists; ++i)
		{
			m_FreeCommandLists.push_back(commandLists[i]);
		}
	}

	ContextType CommandListPool::GetType() const
	{
		return m_Type;
	}

}