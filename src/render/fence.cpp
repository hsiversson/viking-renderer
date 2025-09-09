#include "fence.h"
#include "core/types.h"
#include "device.h"

namespace vkr::Render
{
	FenceResource::FenceResource()
		: m_Value(1)
	{
		GetDevice()->GetD3DDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
	}

	FenceResource::~FenceResource()
	{

	}

	uint64_t FenceResource::Increment()
	{
		return m_Value.fetch_add(1, std::memory_order_acq_rel);
	}

	bool FenceResource::Wait(uint64_t value, bool block) const
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

	bool FenceResource::IsPending(uint64_t value) const
	{
		const uint64_t completedValue = m_Fence->GetCompletedValue();
		if (completedValue == UINT64_MAX)
		{
			// device removed
			VKR_ASSERT(false, "Device was removed");
		}
		return value > completedValue;
	}

	ID3D12Fence* FenceResource::GetFence() const
	{
		return m_Fence.Get();
	}

	uint64_t FenceResource::GetLastValue() const
	{
		return m_Value.load(std::memory_order_acquire) - 1;
	}

	uint64_t FenceResource::GetNextValue() const
	{
		return m_Value.load(std::memory_order_acquire) + 1;
	}

	Fence::Fence()
		: m_FenceResource(nullptr)
		, m_Value(0)
	{
	}

	Fence::Fence(FenceResource* fence, uint64_t value)
		: m_FenceResource(fence)
		, m_Value(value)
	{
	}

	Fence Fence::operator+(uint64_t v) const
	{
		return Fence(m_FenceResource, m_Value + v);
	}

	Fence& Fence::operator+=(uint64_t v)
	{
		m_Value += v;
		return *this;
	}

	bool Fence::Wait(bool block) const
	{
		if (m_FenceResource)
		{
			return m_FenceResource->Wait(m_Value, block);
		}
		return true;
	}

	bool Fence::IsPending() const
	{
		if (m_FenceResource)
		{
			return m_FenceResource->IsPending(m_Value);
		}

		return false;
	}
}