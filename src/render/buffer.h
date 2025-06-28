#pragma once
#include "rendercommon.h"
#include "resource.h"

namespace vkr::Render
{
	struct BufferDesc
	{
		Format m_Format;
		uint32_t m_ElementSize;
		uint32_t m_ElementCount;
		bool m_Writable = false;
		bool m_CpuWritable = false;

		inline uint32_t ByteSize() const { return m_ElementCount * m_ElementSize; }
	};

	class Buffer : public Resource
	{
	public:
		Buffer();
		~Buffer();

		bool Init(const BufferDesc& desc, uint32_t initialDataSize = 0, const void* initialData = nullptr);

		void UploadData(uint64_t offset, uint32_t byteSize, const void* data);
		void DownloadData();

		const BufferDesc& GetDesc() const;

	private:
		BufferDesc m_Desc;
	};

	class TempBufferAllocator
	{
	private:
		struct FrameBlock
		{
			uint64_t start;
			uint64_t end;
			Event event;
		};

	public:
		TempBufferAllocator(uint64_t bufferSizeBytes, uint64_t alignment = 256)
			: m_Capacity(bufferSizeBytes)
			, m_Alignment(alignment)
			, m_FrameStart(UINT64_MAX)
			, m_Head(0)
			, m_Tail(0)
		{
			//m_Blocks.reserve(1024); // grow as needed
		}

		void BeginFrame()
		{
			// Capture where the chunk will start.
			m_FrameStart = m_Head.load(std::memory_order_relaxed);
		}

		constexpr uint64_t Align(uint64_t value, uint64_t alignment)
		{
			return (value + alignment - 1) & ~(alignment - 1);
		}

		uint64_t Allocate(uint64_t size)
		{
			size = Align(size, m_Alignment);

			uint64_t localHead = m_Head.fetch_add(size, std::memory_order_relaxed);
			uint64_t start = localHead % m_Capacity;
			uint64_t end = start + size;

			// Handle wrap‑around in the middle of a frame chunk.
			if (end > m_Capacity)
			{
				// Reserve padding for the spill.
				uint64_t pad = m_Capacity - start;
				localHead = m_Head.fetch_add(pad, std::memory_order_relaxed);
				start = 0;
				// second reservation for the actual data:
				localHead = m_Head.fetch_add(size, std::memory_order_relaxed);
				end = size;
			}

			// Refuse to overwrite data the GPU may still read.
			uint64_t safeTail = m_Tail.load(std::memory_order_acquire);
			if (Distance(safeTail, end) > m_Capacity)
			{
				// Out of space – caller must flush or grow resource.
				m_Head.fetch_sub(size, std::memory_order_relaxed);
				return UINT64_MAX;
			}

			return start;
		}

		void EndFrame(Event event)
		{
			uint64_t chunkEnd = m_Head.load(std::memory_order_relaxed);
			m_Blocks.push_back({ m_FrameStart, chunkEnd, event });
			m_FrameStart = UINT64_MAX; // no open chunk

			GarbageCollect();
		}
		
		void GarbageCollect()
		{
			while (!m_Blocks.empty() && !m_Blocks.front().event.IsPending())
			{
				const FrameBlock& block = m_Blocks.front();
				m_Tail.store(block.end, std::memory_order_release);
				m_Blocks.pop_front();
			}
		}

	private:
		static uint64_t Distance(uint64_t a, uint64_t b) { return b - a; } // Signed distance in ring space

		const uint64_t m_Capacity;
		const uint64_t m_Alignment;
		uint64_t m_FrameStart;

		std::atomic<uint64_t> m_Head;   // producer – many threads
		std::atomic<uint64_t> m_Tail;   // consumer – 1 thread
		std::deque<FrameBlock> m_Blocks;
	};

	struct TempBuffer
	{
		uint64_t m_Offset;
		Buffer* m_Buffer;
	};
}