#include "profiler.h"

#if ENABLE_PROFILING

#include "commandqueue.h"
#include "device.h"
#include "queryheap.h"

namespace vkr::Render
{
	bool Profiler::Init()
	{
		for (uint32_t i = 0; i < CONTEXT_TYPE_COUNT; ++i)
		{
			m_QueryHeaps[i] = MakeRef<QueryHeap>(QUERY_HEAP_TYPE_TIMESTAMP);
		}

		return true;
	}

	void Profiler::BeginFrame(Context* ctx, uint64_t frameIndex)
	{
		const ContextType ctxType = ctx->GetType();
		m_QueryHeaps[ctxType]->ResetIndices();

		PendingFrame newFrame = {};
		newFrame.m_FrameIndex = frameIndex;
		
		PendingEvent frameEvent = {};
		frameEvent.m_Name = std::format("Frame {}", frameIndex);
		frameEvent.m_BeginQueryIndex = m_QueryHeaps[ctxType]->AllocateIndex();
		ctx->TimestampQuery(m_QueryHeaps[ctx->GetType()].get(), frameEvent.m_BeginQueryIndex);

		newFrame.m_RootEventIndex = newFrame.m_Events.size();
		newFrame.m_Events.push_back(std::move(frameEvent));

		m_CurrentFramePerContextType[ctxType] = std::move(newFrame);
		m_CurrentEventIndexPerContextType[ctxType] = newFrame.m_RootEventIndex;
	}

	void Profiler::BeginEvent(Context* ctx, const char* name)
	{
		const ContextType ctxType = ctx->GetType();
		const uint32_t currentEventIndex = m_CurrentEventIndexPerContextType[ctxType];
		PendingEvent& currentEvent = m_CurrentFramePerContextType[ctxType].m_Events[currentEventIndex];

		PendingEvent newEvent = {};
		newEvent.m_Name = name;
		newEvent.m_ParentEventIndex = currentEventIndex;
		newEvent.m_BeginQueryIndex = m_QueryHeaps[ctxType]->AllocateIndex();
		ctx->TimestampQuery(m_QueryHeaps[ctxType].get(), newEvent.m_BeginQueryIndex);

		const uint32_t newEventIndex = m_CurrentFramePerContextType[ctxType].m_Events.size();
		currentEvent.m_ChildEventIndices.push_back(newEventIndex);
		m_CurrentFramePerContextType[ctxType].m_Events.push_back(std::move(newEvent));

		m_CurrentEventIndexPerContextType[ctxType] = newEventIndex;
	}

	void Profiler::EndEvent(Context* ctx)
	{
		const ContextType ctxType = ctx->GetType();
		const uint32_t currentEventIndex = m_CurrentEventIndexPerContextType[ctxType];
		PendingEvent& currentEvent = m_CurrentFramePerContextType[ctxType].m_Events[currentEventIndex];

		currentEvent.m_EndQueryIndex = m_QueryHeaps[ctxType]->AllocateIndex();

		ctx->TimestampQuery(m_QueryHeaps[ctxType].get(), currentEvent.m_EndQueryIndex);

		m_CurrentEventIndexPerContextType[ctxType] = currentEvent.m_ParentEventIndex;
	}

	void Profiler::EndFrame(Context* ctx)
	{
		// Check all pending frames
		const ContextType ctxType = ctx->GetType();
		ResolvePendingFrames(ctxType);

		PendingFrame& pending = m_CurrentFramePerContextType[ctxType];
		const uint32_t currentEventIndex = m_CurrentEventIndexPerContextType[ctxType];
		PendingEvent& currentEvent = m_CurrentFramePerContextType[ctxType].m_Events[currentEventIndex];
		VKR_ASSERT(currentEventIndex == pending.m_RootEventIndex);

		currentEvent.m_EndQueryIndex = m_QueryHeaps[ctxType]->AllocateIndex();

		ctx->TimestampQuery(m_QueryHeaps[ctxType].get(), currentEvent.m_EndQueryIndex);
		ctx->ResolveQueries(m_QueryHeaps[ctxType].get());
		pending.m_Fence = ctx->GetLastFence() + 1;

		m_PendingFramesPerContextType[ctxType].push(std::move(pending));
	}

	const std::queue<ProfilerFrame>& Profiler::GetFrameData(ContextType contextType) const
	{
		return m_ResolvedFramesPerContextType[contextType];
	}

	void Profiler::ResolvePendingFrames(ContextType ctxType)
	{
		uint64_t gpuFrequencyUint = 0;
		GetDevice()->GetCommandQueue(ctxType)->GetD3DCommandQueue()->GetTimestampFrequency(&gpuFrequencyUint);
		const double gpuFrequency = static_cast<double>(gpuFrequencyUint);

		Ref<QueryHeap>& queryHeap = m_QueryHeaps[ctxType];
		const uint64_t* gpuTimestamps = (const uint64_t*)queryHeap->GetBuffer()->GetDataPtr();

		std::queue<PendingFrame>& pendingFrames = m_PendingFramesPerContextType[ctxType];
		while (!pendingFrames.empty())
		{
			PendingFrame& pendingFrame = pendingFrames.front();
			if (!pendingFrame.m_Fence.Wait())
			{
				break;
			}

			ProfilerFrame resolved = {};
			resolved.m_FrameIndex = pendingFrame.m_FrameIndex;
			const PendingEvent& rootEvent = pendingFrame.m_Events[pendingFrame.m_RootEventIndex];
			ResolvePendingEvent(pendingFrame, gpuFrequency, gpuTimestamps, rootEvent, resolved.m_RootEvent);

			m_ResolvedFramesPerContextType[ctxType].push(std::move(resolved));

			if (m_ResolvedFramesPerContextType[ctxType].size() > 64)
				m_ResolvedFramesPerContextType[ctxType].pop();

			pendingFrames.pop();
		}
	}
	void Profiler::ResolvePendingEvent(PendingFrame& pendingFrame, const double gpuFrequency, const uint64_t* gpuTimestamps, const PendingEvent& pendingEvent, ProfilerEvent& resolved)
	{
		const uint64_t startTime = gpuTimestamps[pendingEvent.m_BeginQueryIndex];
		const uint64_t endTime = gpuTimestamps[pendingEvent.m_EndQueryIndex];
		if (endTime > startTime)
		{
			resolved.m_Name = pendingEvent.m_Name;

			uint64_t delta = endTime - startTime;
			resolved.m_ElapsedTimeMs = static_cast<float>(delta / gpuFrequency) * 1000.0;

			for (uint32_t pendingChildEventIndex : pendingEvent.m_ChildEventIndices)
			{
				ProfilerEvent resolvedChildEvent = {};
				const PendingEvent& childEvent = pendingFrame.m_Events[pendingChildEventIndex];
				ResolvePendingEvent(pendingFrame, gpuFrequency, gpuTimestamps, childEvent, resolvedChildEvent);
				resolved.m_ChildEvents.push_back(std::move(resolvedChildEvent));
			}
		}
	}
}

#endif //ENABLE_PROFILING