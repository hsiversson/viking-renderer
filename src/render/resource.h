#pragma once

#include "deviceobject.h"
#include "rendercommon.h"
#include "resourcedescriptor.h"
#include <unordered_map>

namespace vkr::Render
{
	struct ResourceStateTracking
	{
		uint32_t m_CurrentAccess;
		uint32_t m_CurrentSync;
		uint32_t m_CurrentLayout;
	};

	class Resource : public DeviceObject
	{
	public:
		Resource(Device& device);
		bool Init(ID3D12Resource* Resource);

		ID3D12Resource* GetD3DResource() { return m_Resource.Get(); }
		Ref<ResourceDescriptor> GetDescriptor(uint64_t descriptorhash);
		bool AddDescriptor(uint64_t descriptorhash, const Ref<ResourceDescriptor>& descriptor);

	protected:
		ResourceStateTracking m_StateTracking;
		std::unordered_map<uint64_t, Ref<ResourceDescriptor>> m_Descriptors;
		ComPtr<ID3D12Resource> m_Resource; // do we generalize the resources between texture/buffer? We need to keep track of resource state etc, so would be annoying to duplicate.
	};
}