#pragma once
#include "core/types.h"

namespace vkr::Render
{
	class Fence;
	struct Event
	{
		uint64_t m_Value;
		Fence* m_Fence;

		Event();
		Event(Fence* fence, uint64_t value);
		void Wait();
		bool IsPending() const;
	};
}