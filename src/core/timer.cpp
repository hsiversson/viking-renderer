#include "timer.h"

#include <cassert>

namespace vkr
{
	ElapsedTimer* ElapsedTimer::g_Instance = nullptr;

	const ElapsedTimer& ElapsedTimer::Get()
	{
		return *g_Instance;
	}

	ElapsedTimer::ElapsedTimer()
		: m_DeltaTime(0.0)
		, m_ElapsedTime(0.0)
		, m_FrameIndex(0)
		, m_LastTickTime(Time::Now())
	{
		assert(g_Instance == nullptr);
		g_Instance = this;
	}


	ElapsedTimer::~ElapsedTimer()
	{
		g_Instance = nullptr;
	}

	void ElapsedTimer::Tick()
	{
		TimeType now = Time::Now();
		const TimeType durationSeconds = now - m_LastTickTime;
		m_DeltaTime = durationSeconds;
		m_ElapsedTime += durationSeconds;
		m_FrameIndex += 1;

		m_LastTickTime = now;
	}

	uint64_t ElapsedTimer::FrameIndex()
	{
		return Get().m_FrameIndex;
	}

	Timer::Timer()
		: m_StartTime(Time::Now())
	{
	}

	void Timer::Restart()
	{
		m_StartTime = Time::Now();
	}

	float Timer::Stop()
	{
		const TimeType now = Time::Now();
		const TimeType durationSeconds = now - m_StartTime;
		return static_cast<float>(durationSeconds * 1000.0);
	}

}