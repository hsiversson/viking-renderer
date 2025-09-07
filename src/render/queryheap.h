#pragma once

#include "render/rendercommon.h"

namespace vkr::Render
{
	enum QueryHeapType
	{
		QUERY_HEAP_TYPE_TIMESTAMP,
		QUERY_HEAP_TYPE_COPY_TIMESTAMP
	};

	class Buffer;
	class QueryHeap
	{
	public:
		QueryHeap(QueryHeapType type, uint32_t capacity = 1024);
		~QueryHeap();

		uint32_t AllocateIndex();
		void ResetIndices();

		uint32_t GetQueryCount() const;
		uint32_t GetQueryCapacity() const;

		Buffer* GetBuffer() const;

		ID3D12QueryHeap* GetD3DQueryHeap() const;
		QueryHeapType GetType() const;

	private:
		ComPtr<ID3D12QueryHeap> m_QueryHeap;
		Ref<Buffer> m_ResolvedDataBuffer;
		std::atomic<uint32_t> m_QueryCount;

		const uint32_t m_QueryCapacity;
		const QueryHeapType m_Type;
	};
}