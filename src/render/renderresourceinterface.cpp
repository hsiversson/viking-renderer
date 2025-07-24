#include "renderresourceinterface.h"
#include "device.h"

namespace vkr::Render
{
	RenderResourceDestructionQueue* RenderResourceDestructionQueue::g_Instance = nullptr;

	RenderResourceDestructionQueue::RenderResourceDestructionQueue()
		: m_IsRunning(false)
	{
		g_Instance = this;
	}

	RenderResourceDestructionQueue::~RenderResourceDestructionQueue()
	{
		g_Instance = nullptr;
		Stop();
		m_Thread.Wait();
	}

	void RenderResourceDestructionQueue::Start()
	{
		m_IsRunning = true;
		m_Thread.SetName("Render Resource Destruction Queue");
		m_Thread.Start(&RenderResourceDestructionQueue::ThreadFunc, this);
	}

	void RenderResourceDestructionQueue::Stop()
	{
		m_IsRunning = false;
		m_HasWorkEvent.Signal();
	}

	void RenderResourceDestructionQueue::Enqueue(IDeferredDestructibleBase* obj)
	{
		IRenderResource* resource = static_cast<IRenderResource*>(obj);
		assert(resource && "something is wrong, resource is nullptr...");

		PendingResourceDestruction pending = {};
		pending.m_Resource = resource;
		pending.m_Task = QueueGraphicsTask([]() {}, RENDER_TASK_FLAG_WAITABLE_ONLY);

		std::unique_lock<std::recursive_mutex> lock(m_PendingDeletesMutex);
		m_PendingDeletes.push(pending);
		m_HasWorkEvent.Signal();
	}

	void RenderResourceDestructionQueue::Flush()
	{
		std::unique_lock<std::recursive_mutex> lock(m_PendingDeletesMutex);
		PendingResourceDestruction pending;
		while (!m_PendingDeletes.empty())
		{
			pending = m_PendingDeletes.front();
			m_PendingDeletes.pop();
			pending.m_Resource->_OnDestroy();
		}
	}

	RenderResourceDestructionQueue* RenderResourceDestructionQueue::GetInstance()
	{
		return g_Instance;
	}

	void RenderResourceDestructionQueue::ThreadFunc()
	{
		while (m_IsRunning)
		{
			m_HasWorkEvent.Wait();
			m_HasWorkEvent.Reset();

			PendingResourceDestruction pending;
			while (!m_PendingDeletes.empty())
			{
				{
					std::unique_lock<std::recursive_mutex> lock(m_PendingDeletesMutex);
					if (m_PendingDeletes.empty())
						break;

					pending = m_PendingDeletes.front();
					if (pending.m_Task && pending.m_Task->IsPending())
					{
						break;
					}
					m_PendingDeletes.pop();
				}

				pending.m_Resource->_OnDestroy();
			}
		}
	}
}