#include "buffer.h"

#include "device.h"

static const D3D12_HEAP_PROPERTIES UploadHeapProps{ D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
static const D3D12_HEAP_PROPERTIES DefHeapProps{ D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };

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

		ID3D12Resource* resource;
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = 0;
		bufferDesc.Width = desc.m_ElementCount * desc.m_ElementSize;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = desc.bWriteOnGPU ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

		HRESULT hr = GetDevice().GetD3DDevice()->CreateCommittedResource(desc.bWriteOnCPU ? &UploadHeapProps : &DefHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource));
		if (FAILED(hr))
		{
			return false;
		}

		m_Resource = resource;
		InitWithData(initialData, initialDataSize);

		return true;
	}

	bool Buffer::InitWithData(const void* Data, size_t size)
	{
		void* ptr;
		auto res = GetD3DResource();
		res->Map(0, nullptr, &ptr);
		memcpy(ptr, Data, size);
		res->Unmap(0, nullptr);
		return true;
	}

	const BufferDesc& Buffer::GetDesc() const
	{
		return m_Desc;
	}

	void Buffer::UploadData(uint64_t offset, uint32_t byteSize, const void* data)
	{
		static constexpr D3D12_RANGE readRange = { 0 };
		if (m_Desc.bWriteOnCPU)
		{
			// should we keep the resource mapped always if write on cpu is true?

			uint8_t* dataPtr;
			m_Resource->Map(0, &readRange, (void**)&dataPtr);
			memcpy(dataPtr + offset, data, byteSize);
			m_Resource->Unmap(0, nullptr);
		}
		else
		{
			// Create staging buffer
			// map staging buffer and memcpy data
			// run copy operation on a Context and execute (potentially wait as well)
			// discard staging buffer
		}
	}

}