#pragma once
#include "rendercommon.h"
#include <queue>

namespace vkr::Render
{
	class ResourceDescriptor;

	class DescriptorHeap
	{
	public:
		DescriptorHeap();
		~DescriptorHeap();

		void Init(ID3D12DescriptorHeap* heap, uint32_t NumElements, uint32_t descriptorsize);
		ResourceDescriptor* Allocate();
		void Release(ResourceDescriptor* descriptor);

	private:
		ComPtr<ID3D12DescriptorHeap> m_D3DHeap;
		uint32_t m_NumElements;
		uint32_t m_MaxCounter = 0;
		uint32_t m_DescriptorSize;
		std::queue<uint32_t> m_FreeList;
	};
}