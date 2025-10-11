#pragma once
#include "rendercommon.h"
#include "resource.h"

namespace vkr::Render
{
	struct BufferDesc
	{
		Format m_Format = FORMAT_UNKNOWN;
		uint32_t m_ElementSize = 0;
		uint32_t m_ElementCount = 0;
		bool m_Writable = false;
		bool m_CpuWritable = false;
		bool m_IsReadback = false;
		bool m_IsRaytracingAccelerationStructure = false;

		CpuAccess m_CpuAccess = CPU_ACCESS_NONE;
		GpuAccess m_GpuAccess = GPU_ACCESS_READ;

		const char* m_Name = nullptr;

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

	enum TempBufferUsage
	{
		TEMP_BUFFER_USAGE_STAGING,
		TEMP_BUFFER_USAGE_CONSTANTS,
		TEMP_BUFFER_USAGE_SHADER_RESOURCE,
		TEMP_BUFFER_USAGE_RAYTRACING_ACCELERATION_STRUCTURE,

		TEMP_BUFFER_USAGE_COUNT
	};

	struct TempBuffer
	{
		uint64_t m_Offset = 0;
		Ref<Buffer> m_Buffer;
	};

	class TempBufferAllocator
	{
	private:
		struct Chunk
		{
			uint64_t start;
			uint64_t end;
			Fence event;
		};

	public:
		TempBufferAllocator(TempBufferUsage usage, uint64_t bufferSizeBytes, uint64_t alignment = 256);

		void StartChunk();
		bool Allocate(uint64_t size, TempBuffer& outBuf);
		void EndChunk(Fence event);

		uint64_t GetCapacity() const;
		TempBufferUsage GetUsage() const;

	private:
		void GarbageCollect();
		static uint64_t Distance(uint64_t a, uint64_t b) { return b - a; } // Signed distance in ring space

		const uint64_t m_Capacity;
		const uint64_t m_Alignment;
		uint64_t m_ChunkStart;
		std::atomic<bool> m_HasActiveChunk;

		std::atomic<uint64_t> m_Head;   // producer – many threads
		std::atomic<uint64_t> m_Tail;   // consumer – 1 thread
		std::deque<Chunk> m_Chunks;

		Ref<Buffer> m_Buffer;

		const TempBufferUsage m_Usage;
	};
}