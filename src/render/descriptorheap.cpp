#include "descriptorheap.h"

#include "resourcedescriptor.h"

namespace vkr::Render
{
	Ref<ResourceDescriptor> DescriptorHeap::Allocate()
	{
		// TODO: thread safety
		if (m_MaxCounter < m_NumElements)
		{
			uint32_t index = 0;
			if (m_FreeList.empty())
				index = m_MaxCounter++;
			else
			{
				index = m_FreeList.front();
				m_FreeList.pop();
			}
			D3D12_CPU_DESCRIPTOR_HANDLE handle = m_D3DHeap->GetCPUDescriptorHandleForHeapStart();
			handle.ptr += index * m_DescriptorSize;
			auto result = MakeRef<ResourceDescriptor>();
			result->Init(handle, index);
			return result;
		}
		else
		{
			return nullptr;
		}
	}

	void DescriptorHeap::Release(const Ref<ResourceDescriptor> descriptor)
	{
		// TODO: thread safety
		m_FreeList.push(descriptor->GetIndex());
	}

	void DescriptorHeap::Init(ID3D12DescriptorHeap* heap, uint32_t NumElements, uint32_t descriptorsize)
	{
		m_D3DHeap = heap;
		m_NumElements = NumElements;
		m_DescriptorSize = descriptorsize;
	}

	DescriptorHeap::DescriptorHeap()
	{

	}

	DescriptorHeap::~DescriptorHeap()
	{

	}

}
