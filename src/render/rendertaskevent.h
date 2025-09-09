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

		bool Wait(bool block = true) const;
		bool WaitForEvent(bool block = true) const;
		bool WaitForFence(bool block = true) const;

		bool IsPending() const;
		const Fence& GetFence() const { return m_Fence; }

	private:
		Fence m_Fence;
		Event m_Event;
	};
}