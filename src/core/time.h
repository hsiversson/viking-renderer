#pragma once
#include "core/types.h"

namespace vkr
{
	using TimeType = double;

	class Time
	{
	public:
		Time();

		static Time& Get();

		static TimeType Now();

	private:
		TimeType m_Frequency;
	};

}