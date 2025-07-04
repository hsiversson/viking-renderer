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

		uint8_t* GetDataPtr() const;
		const BufferDesc& GetDesc() const;

	private:
		BufferDesc m_Desc;
		uint8_t* m_DataPtr;
	};

	struct TempBuffer
	{
		uint64_t m_Offset;
		Buffer* m_Buffer;
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
		TempBufferAllocator(uint64_t bufferSizeBytes, uint64_t alignment = 256);

		void BeginFrame();
		uint64_t Allocate(uint64_t size);
		void EndFrame(Event event);

	private:
		void GarbageCollect();
		static uint64_t Distance(uint64_t a, uint64_t b) { return b - a; } // Signed distance in ring space

		const uint64_t m_Capacity;
		const uint64_t m_Alignment;
		uint64_t m_FrameStart;

		std::atomic<uint64_t> m_Head;   // producer – many threads
		std::atomic<uint64_t> m_Tail;   // consumer – 1 thread
		std::deque<FrameBlock> m_Blocks;
	};
}