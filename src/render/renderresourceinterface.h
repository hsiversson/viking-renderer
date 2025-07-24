#pragma once
#include "core/types.h"
#include "core/event.h"
#include "rendertaskevent.h"

namespace vkr::Render
{
	class RenderResourceDestructionQueue;
	struct IRenderResource : public IDeferredDestructible<IRenderResource, RenderResourceDestructionQueue>
	{
	};

	class RenderResourceDestructionQueue final : public IDeferredDestructionQueue
	{
	public:
		RenderResourceDestructionQueue();
		~RenderResourceDestructionQueue();
		void Start();
		void Stop();

		void Enqueue(IDeferredDestructibleBase* obj) override;
		void Flush() override;

		static RenderResourceDestructionQueue* GetInstance();

	private:
		void ThreadFunc();

		struct PendingResourceDestruction
		{
			IRenderResource* m_Resource;
			Ref<RenderTaskEvent> m_Task;
		};

		std::recursive_mutex m_PendingDeletesMutex;
		std::queue<PendingResourceDestruction> m_PendingDeletes;
		Event m_HasWorkEvent;

		Thread m_Thread;
		bool m_IsRunning;

		static RenderResourceDestructionQueue* g_Instance;
	};
}