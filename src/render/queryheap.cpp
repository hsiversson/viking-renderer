#include "queryheap.h"

#include "device.h"

namespace vkr::Render
{
    QueryHeap::QueryHeap(QueryHeapType type, uint32_t capacity)
        : m_QueryCapacity(capacity)
        , m_Type(type)
    {
        D3D12_QUERY_HEAP_DESC desc = {};
        desc.Count = capacity;

        switch (m_Type)
        {
        case QUERY_HEAP_TYPE_TIMESTAMP:
            desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
            break;
        case QUERY_HEAP_TYPE_COPY_TIMESTAMP:
            desc.Type = D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP;
            break;
        default:
            VKR_CHECK_NO_ENTRY();
            return;
        }

        GetDevice()->GetD3DDevice()->CreateQueryHeap(&desc, IID_PPV_ARGS(&m_QueryHeap));

        BufferDesc bufferDesc = {};
        bufferDesc.m_Format = FORMAT_UNKNOWN;
        bufferDesc.m_IsReadback = true;
        bufferDesc.m_ElementCount = capacity;
        bufferDesc.m_ElementSize = sizeof(uint64_t);
        m_ResolvedDataBuffer = GetDevice()->CreateBuffer(bufferDesc);
    }

    QueryHeap::~QueryHeap()
    {
    }

    uint32_t QueryHeap::AllocateIndex()
    {
        return m_QueryCount.fetch_add(1, std::memory_order_acq_rel);
    }

    void QueryHeap::ResetIndices()
    {
        m_QueryCount.store(0, std::memory_order_release);
    }

    uint32_t QueryHeap::GetQueryCount() const
    {
        return m_QueryCount;
    }

    uint32_t QueryHeap::GetQueryCapacity() const
    {
        return m_QueryCapacity;
    }

    Buffer* QueryHeap::GetBuffer() const
    {
        return m_ResolvedDataBuffer.get();
    }

    ID3D12QueryHeap* QueryHeap::GetD3DQueryHeap() const
    {
        return m_QueryHeap.Get();
    }

    QueryHeapType QueryHeap::GetType() const
    {
        return m_Type;
    }
}