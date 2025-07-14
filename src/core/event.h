#pragma once
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace vkr
{
	class Event
	{
	public:
		Event();
		~Event() = default;

		void Signal();
		void Reset();
		bool Wait(bool block = true) const;

		bool IsSignalled() const;

	private:
		mutable std::mutex m_Mutex;
		mutable std::condition_variable m_ConditionVariable;
		mutable std::atomic<bool> m_Signalled;
	};
}