#pragma once

#include "rendercommon.h"
#include "resourcedescriptor.h"
#include <unordered_map>

namespace vkr::Render
{
	class Resource
	{
	public:
		bool Init(ID3D12Resource* Resource);

		ID3D12Resource* GetD3DResource() { return m_Resource.Get(); }
		ResourceDescriptor* GetDescriptor(uint64_t descriptorhash);
		bool AddDescriptor(uint64_t descriptorhash, ResourceDescriptor* descriptor);
	private:
		std::unordered_map<uint64_t, ResourceDescriptor*> m_Descriptors;
		ComPtr<ID3D12Resource> m_Resource; // do we generalize the resources between texture/buffer? We need to keep track of resource state etc, so would be annoying to duplicate.
	};
}