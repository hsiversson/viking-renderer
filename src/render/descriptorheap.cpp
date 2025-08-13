#include "descriptorheap.h"

#include "resourcedescriptor.h"
#include "device.h"

namespace vkr::Render
{
	DescriptorHeap::DescriptorHeap(DescriptorHeapType type, uint32_t numDescriptors)
		: m_Type(type)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = numDescriptors;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		switch (type)
		{
		case DESCRIPTOR_HEAP_TYPE_SHADER_RESOURCE:
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			break;
		case DESCRIPTOR_HEAP_TYPE_SAMPLER:
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			break;
		case DESCRIPTOR_HEAP_TYPE_RENDER_TARGET:
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			break;
		case DESCRIPTOR_HEAP_TYPE_DEPTH_STENCIL:
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			break;
		default:
			VKR_CHECK_NO_ENTRY();
			return;
		}

		HRESULT hr = GetDevice()->GetD3DDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_D3DHeap));
		if (FAILED(hr))
		{
			VKR_ASSERT(false);
		}

		m_DescriptorSize = GetDevice()->GetD3DDevice()->GetDescriptorHandleIncrementSize(desc.Type);
		m_NumElements = numDescriptors;
	}

	DescriptorHeap::~DescriptorHeap()
	{

	}

	void DescriptorHeap::Allocate(ResourceDescriptor& descriptor)
	{
		// TODO: thread safety
		if (m_MaxCounter < m_NumElements)
		{
			uint32_t index = 0;
			{
				std::unique_lock<std::mutex> lock(m_FreeListMutex);
				if (m_FreeList.empty())
					index = m_MaxCounter++;
				else
				{
					index = m_FreeList.front();
					m_FreeList.pop();
				}
			}

			descriptor.m_D3DHandle = m_D3DHeap->GetCPUDescriptorHandleForHeapStart();
			descriptor.m_D3DHandle.ptr += index * m_DescriptorSize;
			descriptor.m_DescriptorIndex = index;
		}
		else
		{
			VKR_ASSERT(false && "Descriptor heap full.");
			return;
		}
	}

	void DescriptorHeap::Release(const ResourceDescriptor& descriptor)
	{
		// TODO: thread safety
		std::unique_lock<std::mutex> lock(m_FreeListMutex);
		m_FreeList.push(descriptor.GetIndex());
	}

	uint32_t DescriptorHeap::GetDescriptorSize() const
	{
		return m_DescriptorSize;
	}

	ID3D12DescriptorHeap* DescriptorHeap::GetD3DDescriptorHeap() const
	{
		return m_D3DHeap.Get();
	}
}
