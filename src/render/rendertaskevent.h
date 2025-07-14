#pragma once
#include "core/types.h"
#include "core/event.h"
#include "render/fence.h"

namespace vkr::Render
{
	class RenderTaskEvent
	{
		friend class RenderThread;
	public:
		RenderTaskEvent();
		~RenderTaskEvent() = default;

		bool Wait(bool block = true);
		bool WaitForEvent(bool block = true);
		bool WaitForFence(bool block = true);

	private:
		Fence m_Fence;
		Event m_Event;
	};
}