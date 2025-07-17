#include "renderthread.h"
#include "device.h"

namespace vkr::Render
{
	RenderThread::RenderThread(ContextType type)
		: m_IsRunning(false)
		, m_ContextType(type)
	{
	}

	RenderThread::~RenderThread()
	{
	}

	void RenderThread::Start()
	{
		if (!m_IsRunning)
		{
			m_IsRunning = true;
			m_Thread = std::thread(&RenderThread::ThreadFunc, this);
		}
	}

	void RenderThread::Stop()
	{
		if (m_IsRunning)
		{
			m_IsRunning = false;
			m_HasWorkEvent.Signal();
			m_Thread.join();
		}
	}

	Ref<RenderTaskEvent> RenderThread::QueueTask(RenderTaskFn task)
	{
		if (!m_IsRunning)
			return nullptr;

		RenderTask renderTask;
		renderTask.m_Task = task;
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
		Ref<Context> ctx = GetDevice()->GetContext(m_ContextType);
		while (m_IsRunning)
		{
			m_HasWorkEvent.Wait();
			m_HasWorkEvent.Reset();

			RenderTask task;
			while (m_IsRunning && !m_PendingTasks.empty())
			{
				{
					std::unique_lock<std::mutex> lock(m_PendingTasksMutex);
					task = m_PendingTasks.front();
					m_PendingTasks.pop();
				}

				ctx->Begin();
				task.m_Task();
				ctx->End();
				task.m_Event->m_Fence = ctx->Flush();
				task.m_Event->m_Event.Signal();
			}
		}
	}
}