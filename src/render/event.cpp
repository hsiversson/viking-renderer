#include "event.h"
#include "fence.h"

namespace vkr::Render
{
	Event::Event()
		: m_Fence(nullptr)
		, m_Value(0)
	{
	}

	Event::Event(Fence* fence, uint64_t value)
		: m_Fence(fence)
		, m_Value(value)
	{
	}

	void Event::Wait()
	{
		if (m_Fence)
		{
			m_Fence->Wait(m_Value);
		}
	}

	bool Event::IsPending() const
	{
		if (m_Fence)
		{
			return m_Fence->IsPending(m_Value);
		}

		return false;
	}
}