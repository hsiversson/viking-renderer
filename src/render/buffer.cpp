#include "buffer.h"

#include "device.h"
#include "d3dconvert.h"

namespace vkr::Render
{
	Buffer::Buffer()
		: m_DataPtr(nullptr)
	{

	}

	Buffer::~Buffer()
	{

	}

	bool Buffer::Init(const BufferDesc& desc, uint32_t initialDataSize /*= 0*/, const void* initialData /*= nullptr*/)
	{
		m_Desc = desc;

		//For now allocate resource in place. Later well see how we do pooling

		const D3D12_RESOURCE_DESC1 bufferDesc = D3DConvertBufferDesc(desc);
		const D3D12_HEAP_PROPERTIES& heapProps = desc.m_CpuWritable ? D3DGetUploadHeapProperties() : D3DGetDefaultHeapProperties();
		HRESULT hr = GetDevice()->GetD3DDevice10()->CreateCommittedResource3(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, nullptr, 0, nullptr, IID_PPV_ARGS(&m_Resource));
		if (FAILED(hr))
		{
			return false;
		}

		m_StateTracking.m_CurrentAccess = RESOURCE_STATE_ACCESS_COMMON;
		m_StateTracking.m_CurrentSync = RESOURCE_STATE_SYNC_ALL;
		m_StateTracking.m_CurrentLayout = RESOURCE_STATE_LAYOUT_UNDEFINED;

		if (desc.m_CpuWritable)
		{
			// Always keep buffers on the upload heap mapped for write.
			static constexpr D3D12_RANGE readRange = { 0 };
			m_Resource->Map(0, &readRange, (void**)&m_DataPtr);
		}

		if (initialData)
		{
			UploadData(0, initialDataSize, initialData);
		}

		return true;
	}

	uint8_t* Buffer::GetDataPtr() const
	{
		return m_DataPtr;
	}

	const BufferDesc& Buffer::GetDesc() const
	{
		return m_Desc;
	}

	void Buffer::UploadData(uint64_t offset, uint32_t byteSize, const void* data)
	{
		if (m_DataPtr)
		{
			memcpy(m_DataPtr + offset, data, byteSize);
		}
		//else if (isRenderThread)
		//{
		//	// Create staging buffer
		//	// map staging buffer and memcpy data
		//	// run copy operation on a Context and execute (potentially wait as well)
		//	// discard staging buffer
		//}
		//else
		//{
		//	// Launch copy task on render thread, wait for event?
		//}
	}

	TempBufferAllocator::TempBufferAllocator(uint64_t bufferSizeBytes, uint64_t alignment)
		: m_Capacity(bufferSizeBytes)
		, m_Alignment(alignment)
		, m_ChunkStart(UINT64_MAX)
		, m_Head(0)
		, m_Tail(0)
	{
		BufferDesc tempBufferDesc = {};
		tempBufferDesc.m_CpuWritable = true;
		tempBufferDesc.m_Writable = false;
		tempBufferDesc.m_ElementCount = bufferSizeBytes;
		tempBufferDesc.m_ElementSize = 1;
		tempBufferDesc.m_Format = FORMAT_UNKNOWN;
		m_Buffer = GetDevice()->CreateBuffer(tempBufferDesc);
	}

	void TempBufferAllocator::StartChunk()
	{
		// Capture where the chunk will start.
		m_ChunkStart = m_Head.load(std::memory_order_relaxed);
	}

	bool TempBufferAllocator::Allocate(uint64_t size, TempBuffer& outBuf)
	{
		size = Align(size, m_Alignment);

		while (true)
		{
			uint64_t oldHead = m_Head.load(std::memory_order_relaxed);
			uint64_t start = oldHead;
			uint64_t end = start + size;

			uint64_t startMod = start % m_Capacity;
			if (startMod + size > m_Capacity)
			{
				uint64_t pad = m_Capacity - startMod;
				start += pad;
				end += pad;
				startMod = 0;
			}

			uint64_t safeTail = m_Tail.load(std::memory_order_acquire);
			if (end - safeTail > m_Capacity)
				return false;                  // out of space, caller must flush

			if (m_Head.compare_exchange_weak(oldHead, end, std::memory_order_release, std::memory_order_relaxed))
			{
				outBuf.m_Offset = startMod;
				outBuf.m_Buffer = m_Buffer.get();
				return true;
			}
		}
	}

	void TempBufferAllocator::EndChunk(Event event)
	{
		uint64_t end = m_Head.load(std::memory_order_relaxed);
		m_Chunks.push_back({ m_ChunkStart, end, event });
		m_ChunkStart = UINT64_MAX;

		GarbageCollect();
	}
	
	uint64_t TempBufferAllocator::GetCapacity() const
	{
		return m_Capacity;
	}

	void TempBufferAllocator::GarbageCollect()
	{
		while (!m_Chunks.empty() && !m_Chunks.front().event.IsPending())
		{
			const Chunk& chunk = m_Chunks.front();
			m_Tail.store(chunk.end, std::memory_order_release);
			m_Chunks.pop_front();
		}
	}
}