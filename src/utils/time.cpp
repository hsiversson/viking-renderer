#include "time.h"

#include <windows.h>

namespace vkr
{

	Time::Time()
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		m_Frequency = static_cast<TimeType>(freq.QuadPart);
	}

	Time& Time::Get()
	{
		static Time instance;
		return instance;
	}

	TimeType Time::Now()
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return static_cast<TimeType>(now.QuadPart) / Get().m_Frequency;
	}
}

