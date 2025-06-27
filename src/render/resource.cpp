#include "resource.h"

namespace vkr::Render
{
	bool Resource::InitWithResource(ID3D12Resource* resource)
	{
		m_Resource = resource;
		return true;
	}

	void Resource::SetName(const char* name)
	{
		if (m_Resource)
		{
			std::wstring wName = UTF8ToUTF16(name);
			m_Resource->SetName(wName.c_str());
		}		
	}

	bool Resource::TrackDescriptor(uint64_t hash, const WeakPtr<ResourceDescriptor>& descriptor)
	{
		auto it = m_Descriptors.find(hash);
		if (it != m_Descriptors.end())
			return false;
		m_Descriptors[hash] = descriptor;
		return true;
	}

	Ref<ResourceDescriptor> Resource::GetDescriptor(uint64_t hash)
	{
		auto it = m_Descriptors.find(hash);
		if (it == m_Descriptors.end())
		{
			return nullptr;
		}
			
		if (it->second.expired())
		{
			m_Descriptors.erase(it);
			return nullptr;
		}

		return it->second.lock();
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