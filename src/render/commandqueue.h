#pragma once 
#include "context.h"
#include "rendertaskevent.h"
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

		Fence Signal();
		void InsertWait(const Fence& fence);
		bool Wait(bool block = true);

		Fence Submit(const Ref<CommandList>& commandList);
		Fence Submit(uint32_t numCommandLists, const Ref<CommandList>* commandLists);

		Fence GetNextFence() const;
		Fence GetLastFence() const;
		ID3D12CommandQueue* GetD3DCommandQueue() const;

	private:
		ComPtr<ID3D12CommandQueue> m_CommandQueue;
		UniquePtr<FenceResource> m_FenceResource;
		const ContextType m_Type;
	};
}