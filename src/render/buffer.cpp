#include "buffer.h"

#include "device.h"

static constexpr D3D12_HEAP_PROPERTIES g_UploadHeapProps{ D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
static constexpr D3D12_HEAP_PROPERTIES g_DefaultHeapProps{ D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };

namespace vkr::Render
{
	Buffer::Buffer()
	{

	}

	Buffer::~Buffer()
	{

	}

	bool Buffer::Init(const BufferDesc& desc, uint32_t initialDataSize /*= 0*/, const void* initialData /*= nullptr*/)
	{
		m_Desc = desc;

		//For now allocate resource in place. Later well see how we do pooling

		D3D12_RESOURCE_DESC1 bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = 0;
		bufferDesc.Width = desc.ByteSize();
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = desc.m_Writable ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

		const D3D12_HEAP_PROPERTIES& heapProps = desc.m_CpuWritable ? g_UploadHeapProps : g_DefaultHeapProps;
		HRESULT hr = GetDevice().GetD3DDevice10()->CreateCommittedResource3(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, nullptr, 0, nullptr, IID_PPV_ARGS(&m_Resource));
		if (FAILED(hr))
		{
			return false;
		}

		if (initialData)
		{
			UploadData(0, initialDataSize, initialData);
		}

		m_StateTracking.m_CurrentAccess = RESOURCE_STATE_ACCESS_COMMON;
		m_StateTracking.m_CurrentSync = RESOURCE_STATE_SYNC_ALL;
		m_StateTracking.m_CurrentLayout = RESOURCE_STATE_LAYOUT_UNDEFINED;
		return true;
	}

	const BufferDesc& Buffer::GetDesc() const
	{
		return m_Desc;
	}

	void Buffer::UploadData(uint64_t offset, uint32_t byteSize, const void* data)
	{
		static constexpr D3D12_RANGE readRange = { 0 };
		if (m_Desc.m_CpuWritable)
		{
			// should we keep the resource mapped always if write on cpu is true?

			uint8_t* dataPtr;
			m_Resource->Map(0, &readRange, (void**)&dataPtr);
			memcpy(dataPtr + offset, data, byteSize);
			m_Resource->Unmap(0, nullptr);
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

}