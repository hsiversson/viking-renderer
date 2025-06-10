#include "resource.h"

namespace vkr::Render
{
	bool Resource::InitWithResource(ID3D12Resource* resource)
	{
		m_Resource = resource;
		return true;
	}

	bool Resource::AddDescriptor(uint64_t descriptorhash, const Ref<ResourceDescriptor>& descriptor)
	{
		auto it = m_Descriptors.find(descriptorhash);
		if (it != m_Descriptors.end())
			return false;
		m_Descriptors[descriptorhash] = descriptor;
		return true;
	}

	Ref<ResourceDescriptor> Resource::GetDescriptor(uint64_t descriptorhash)
	{
		auto it = m_Descriptors.find(descriptorhash);
		return it == m_Descriptors.end() ? nullptr : it->second;
	}

	Resource::Resource()
	{

	}

	ResourceStateTracking& Resource::GetStateTracking()
	{
		return m_StateTracking;
	}

	const ResourceStateTracking& Resource::GetStateTracking() const
	{
		return m_StateTracking;
	}

	void Resource::SetGpuPending(Event event)
	{
		SyncGpu();
		m_GpuPendingEvent = event;
	}

	bool Resource::IsGpuPending() const
	{
		return m_GpuPendingEvent.IsPending();
	}

	void Resource::SyncGpu()
	{
		m_GpuPendingEvent.Wait();
	}

}