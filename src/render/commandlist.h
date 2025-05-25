#pragma once
#include "render/deviceobject.h"
#include "render/rendercommon.h"
#include "render/context.h"

namespace vkr::Render
{
	class CommandList : public DeviceObject
	{
	public:
		CommandList(Device& device, ContextType type);

		void Open();
		void Close();

		ContextType GetType() const;

		ID3D12GraphicsCommandList* GetD3DCommandList() const;

	private:
		ComPtr<ID3D12GraphicsCommandList> m_CommandList;
		ComPtr<ID3D12CommandAllocator> m_Allocator;
		const ContextType m_Type;
	};

	class CommandListPool : public DeviceObject
	{
	public:
		CommandListPool(Device& device, ContextType type);
		~CommandListPool();

		Ref<CommandList> GetCommandList();
		void ReturnCommandList(const Ref<CommandList>& commandList);
		void ReturnCommandList(uint32_t numCommandLists, const Ref<CommandList>* commandLists);

		ContextType GetType() const;

	private:
		std::vector<Ref<CommandList>> m_FreeCommandLists;
		std::mutex m_Mutex;

		const ContextType m_Type;
	};
}