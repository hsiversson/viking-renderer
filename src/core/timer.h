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

		float DeltaTime() const { return static_cast<float>(m_DeltaTime); }
		float ElapsedTime() const { return static_cast<float>(m_ElapsedTime); }
		uint64_t FrameIndex() const;

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