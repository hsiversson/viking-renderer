#pragma once
#if ENABLE_PROFILING

#include "context.h"
#include "device.h"

namespace vkr::Render
{
	struct ProfilerEvent
	{
		std::string m_Name;
		float m_ElapsedTimeMs;
		std::vector<ProfilerEvent> m_ChildEvents;
	};

	struct ProfilerFrame
	{
		uint64_t m_FrameIndex;
		ProfilerEvent m_RootEvent;
	};

	class Profiler
	{
	public:
		Profiler() = default;
		~Profiler() = default;

		bool Init();

		void BeginFrame(Context* ctx, uint64_t frameIndex);

		void BeginEvent(Context* ctx, const char* name);
		void EndEvent(Context* ctx);

		void EndFrame(Context* ctx);

		const std::queue<ProfilerFrame>& GetFrameData(ContextType contextType) const;

	private:
		struct PendingEvent
		{
			std::string m_Name;
			uint32_t m_BeginQueryIndex;
			uint32_t m_EndQueryIndex;
			uint32_t m_ParentEventIndex;
			std::vector<uint32_t> m_ChildEventIndices;
		};
		struct PendingFrame
		{
			uint64_t m_FrameIndex;
			uint32_t m_RootEventIndex;
			Fence m_Fence;
			std::vector<PendingEvent> m_Events;
		};

		void ResolvePendingFrames(ContextType ctxType);
		void ResolvePendingEvent(PendingFrame& pendingFrame, const double gpuFrequency, const uint64_t* gpuTimestamps, const PendingEvent& pendingEvent, ProfilerEvent& resolved);

	private:
		std::array<Ref<QueryHeap>, CONTEXT_TYPE_COUNT> m_QueryHeaps;

		std::array<PendingFrame, CONTEXT_TYPE_COUNT> m_CurrentFramePerContextType;
		std::array<uint32_t, CONTEXT_TYPE_COUNT> m_CurrentEventIndexPerContextType;

		std::array<std::queue<PendingFrame>, CONTEXT_TYPE_COUNT> m_PendingFramesPerContextType;
		std::array<std::queue<ProfilerFrame>, CONTEXT_TYPE_COUNT> m_ResolvedFramesPerContextType;
	};
}
#endif // ENABLE_PROFILING