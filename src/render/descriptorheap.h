#pragma once
#include "rendercommon.h"
#include <queue>

namespace vkr::Render
{
	class ResourceDescriptor;

	enum DescriptorHeapType
	{
		DESCRIPTOR_HEAP_TYPE_SHADER_RESOURCE,
		DESCRIPTOR_HEAP_TYPE_SAMPLER,
		DESCRIPTOR_HEAP_TYPE_RENDER_TARGET,
		DESCRIPTOR_HEAP_TYPE_DEPTH_STENCIL,

		DESCRIPTOR_HEAP_TYPE_COUNT
	};

	class DescriptorHeap
	{
	public:
		DescriptorHeap(DescriptorHeapType type, uint32_t numDescriptors);
		~DescriptorHeap();

		void Allocate(ResourceDescriptor& descriptor);
		void Release(const ResourceDescriptor& descriptor);

		uint32_t GetDescriptorSize() const;

	private:
		ComPtr<ID3D12DescriptorHeap> m_D3DHeap;
		uint32_t m_NumElements;
		uint32_t m_MaxCounter = 0;
		uint32_t m_DescriptorSize;
		std::queue<uint32_t> m_FreeList;

		const DescriptorHeapType m_Type;
	};
}