#pragma once 
#include "context.h"
#include "event.h"
#include "fence.h"

struct ID3D12CommandQueue;

namespace vkr::Render
{
	class CommandList;
	class CommandQueue
	{
	public:
		CommandQueue(ContextType type);
		~CommandQueue();

		Event Signal();
		void InsertWait(const Event& waitable);
		bool Wait(bool block = true);

		Event Submit(const Ref<CommandList>& commandList);
		Event Submit(uint32_t numCommandLists, const Ref<CommandList>* commandLists);

		ID3D12CommandQueue* GetD3DCommandQueue() const;

	private:
		ComPtr<ID3D12CommandQueue> m_CommandQueue;
		UniquePtr<Fence> m_Fence;
		const ContextType m_Type;
	};
}