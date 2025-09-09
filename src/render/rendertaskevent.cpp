#include "rendertaskevent.h"
#include "fence.h"

namespace vkr::Render
{

	RenderTaskEvent::RenderTaskEvent()
	{

	}

	bool RenderTaskEvent::Wait(bool block /*= true*/) const
	{
		if (!block)
		{
			if (!m_Event.Wait(false)) 
				return false;
			if (!m_Fence.Wait(false))
				return false;
			return true;
		}

		m_Event.Wait(true);
		m_Fence.Wait(true);
		return true;
	}

	bool RenderTaskEvent::WaitForEvent(bool block) const
	{
		return m_Event.Wait(block);
	}

	bool RenderTaskEvent::WaitForFence(bool block) const
	{
		return m_Fence.Wait(block);
	}

	bool RenderTaskEvent::IsPending() const
	{
		return !m_Event.IsSignalled() || m_Fence.IsPending();
	}
}