#include "renderthread.h"
#include "device.h"

namespace vkr::Render
{
	RenderThread::RenderThread(ContextType type)
		: m_IsRunning(false)
		, m_ContextType(type)
	{
		switch (m_ContextType)
		{
		case CONTEXT_TYPE_GRAPHICS:
			m_Thread.SetName("Graphics Render Thread");
			break;
		case CONTEXT_TYPE_COMPUTE:
			m_Thread.SetName("Compute Render Thread");
			break;
		case CONTEXT_TYPE_COPY:
			m_Thread.SetName("Copy Render Thread");
			break;
		}
	}

	RenderThread::~RenderThread()
	{
	}

	void RenderThread::Start()
	{
		if (!m_IsRunning)
		{
			m_IsRunning = true;
			m_Thread.Start(&RenderThread::ThreadFunc, this);
		}
	}

	void RenderThread::Stop()
	{
		if (m_IsRunning)
		{
			m_IsRunning = false;
			m_HasWorkEvent.Signal();
			m_Thread.Wait();
		}
	}

	Ref<RenderTaskEvent> RenderThread::QueueTask(RenderTaskFn task, RenderTaskFlags flags)
	{
		if (!m_IsRunning)
			return nullptr;

		RenderTask renderTask;
		renderTask.m_Task = task;
		renderTask.m_Flags = flags;
		renderTask.m_Event = MakeRef<RenderTaskEvent>();

		{
			std::unique_lock<std::mutex> lock(m_PendingTasksMutex);
			m_PendingTasks.push(renderTask);
		}

		m_HasWorkEvent.Signal();
		return renderTask.m_Event;
	}

	void RenderThread::ThreadFunc()
	{
		Thread::RegisterRenderThread();
		Ref<Context> ctx = GetDevice()->GetContext(m_ContextType);
		while (m_IsRunning)
		{
			m_HasWorkEvent.Wait();
			m_HasWorkEvent.Reset();

			RenderTask task;
			while (!m_PendingTasks.empty())
			{
				{
					std::unique_lock<std::mutex> lock(m_PendingTasksMutex);
					task = m_PendingTasks.front();
					m_PendingTasks.pop();
				}

				if (task.m_Flags & RENDER_TASK_FLAG_WAITABLE_ONLY)
				{
					task.m_Event->m_Fence = ctx->GetLastFence();
				}
				else
				{
					ctx->Begin();
					task.m_Task();
					ctx->End();
					task.m_Event->m_Fence = ctx->Flush();
				}
				task.m_Event->m_Event.Signal();
			}
		}
	}
}