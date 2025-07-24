#include "event.h"

namespace vkr
{
	Event::Event()
		: m_Signalled(false)
	{
	}

	void Event::Signal()
	{
		m_Signalled.store(true, std::memory_order_release);
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_ConditionVariable.notify_all();
	}

	void Event::Reset()
	{
		m_Signalled.store(false, std::memory_order_release);
	}

	bool Event::Wait(bool block /*= true*/) const
	{
		if (!block)
			return m_Signalled.load(std::memory_order_acquire);

		// Spin a few times to avoid kernel calls on fast wakeups
		for (uint32_t i = 0; i < 64; ++i)
		{
			if (m_Signalled.load(std::memory_order_acquire))
				return true;
			Thread::Yield();
		}

		std::unique_lock<std::mutex> lock(m_Mutex);
		m_ConditionVariable.wait(lock, [&]() { return m_Signalled.load(std::memory_order_acquire); });
		return true;
	}

	bool Event::IsSignalled() const
	{
		return m_Signalled.load(std::memory_order_acquire);
	}

}