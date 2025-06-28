#include "fence.h"
#include "core/types.h"
#include "device.h"

namespace vkr::Render
{

	Fence::Fence()
		: m_Value(1)
	{
		GetDevice().GetD3DDevice()->CreateFence(m_Value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
	}

	Fence::~Fence()
	{

	}

	uint64_t Fence::Increment()
	{
		return m_Value.fetch_add(1);
	}

	bool Fence::Wait(uint64_t value, bool block)
	{
		if (IsPending(value))
		{
			if (!block)
			{
				return false;
			}
			HANDLE event = CreateEvent(nullptr, false, false, nullptr);
			m_Fence->SetEventOnCompletion(value, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		return true;
	}

	bool Fence::IsPending(uint64_t value) const
	{
		return value > m_Fence->GetCompletedValue();
	}

	ID3D12Fence* Fence::GetFence() const
	{
		return m_Fence.Get();
	}

	uint64_t Fence::GetLastValue() const
	{
		return m_Value - 1;
	}

}