#pragma once
#include "time.h"

namespace vkr
{
	class ElapsedTimer
	{
	public:
		static const ElapsedTimer& Get();

		ElapsedTimer();
		~ElapsedTimer();

		void Tick();

		static float DeltaTime() { return static_cast<float>(Get().m_DeltaTime); }
		static float ElapsedTime() { return static_cast<float>(Get().m_ElapsedTime); }
		static uint64_t FrameIndex();

	private:
		double m_DeltaTime;
		double m_ElapsedTime;
		uint64_t m_FrameIndex;

		TimeType m_LastTickTime;

		static ElapsedTimer* g_Instance;
	};

	class Timer
	{
	public:
		Timer();
		void Restart();
		float Stop(); // Returns the number of milliseconds that have passed since timer started.
	private:
		TimeType m_StartTime;
	};
}