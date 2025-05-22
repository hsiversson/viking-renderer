#include "resource.h"

namespace vkr::Render
{
	bool Resource::Init(ID3D12Resource* resource)
	{
		m_Resource = resource;
		return true;
	}

	bool Resource::AddDescriptor(uint64_t descriptorhash, ResourceDescriptor* descriptor)
	{
		auto it = m_Descriptors.find(descriptorhash);
		if (it != m_Descriptors.end())
			return false;
		m_Descriptors[descriptorhash] = descriptor;
		return true;
	}

	ResourceDescriptor* Resource::GetDescriptor(uint64_t descriptorhash)
	{
		auto it = m_Descriptors.find(descriptorhash);
		return it == m_Descriptors.end() ? nullptr : it->second;
	}

	Resource::Resource(Device& device) :
		DeviceObject(device)
	{

	}

}