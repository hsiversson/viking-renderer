#pragma once
#include "core/thread.h"
#include "context.h"
#include "rendertaskevent.h"

namespace vkr::Render
{
	using RenderTaskFn = std::function<void()>;

	enum RenderTaskFlags
	{
		RENDER_TASK_FLAG_NONE			= 0,
		RENDER_TASK_FLAG_WAITABLE_ONLY	= (1<<0),
		RENDER_TASK_FLAG_FORCE_FLUSH	= (1<<1),
	};

	struct RenderTask
	{
		RenderTaskFn m_Task;
		RenderTaskFlags m_Flags;
		Ref<RenderTaskEvent> m_Event;
	};

	class RenderThread
	{
	public:
		RenderThread(ContextType type);
		~RenderThread();
		void Start();
		void Stop();

		Ref<RenderTaskEvent> QueueTask(RenderTaskFn task, RenderTaskFlags flags = RENDER_TASK_FLAG_NONE);

	private:
		void ThreadFunc();

		std::mutex m_PendingTasksMutex;
		std::queue<RenderTask> m_PendingTasks;
		Event m_HasWorkEvent;

		Thread m_Thread;
		bool m_IsRunning;

		const ContextType m_ContextType;
	};
}