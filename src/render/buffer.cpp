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

		HRESULT hr = GetDevice().GetD3DDevice()->CreateCommittedResource(desc.bWriteOnCPU ? &UploadHeapProps : &DefHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource));
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

}