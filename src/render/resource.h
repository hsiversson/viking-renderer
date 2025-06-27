#pragma once

#include "rendercommon.h"
#include "resourcedescriptor.h"
#include <unordered_map>
#include "event.h"

namespace vkr::Render
{
	enum class ResourceDimension
	{
		Unknown,
		Buffer,
		Texture1D,
		Texture2D,
		Texture3D
	};

	struct ResourceStateTracking
	{
		ResourceStateAccess m_CurrentAccess;
		ResourceStateSync m_CurrentSync;
		ResourceStateLayout m_CurrentLayout;
	};

	class Resource
	{
	public:
		Resource();
		bool InitWithResource(ID3D12Resource* Resource);

		void SetName(const char* name);

		ID3D12Resource* GetD3DResource() { return m_Resource.Get(); }
		Ref<ResourceDescriptor> GetDescriptor(uint64_t descriptorhash);
		bool AddDescriptor(uint64_t descriptorhash, const Ref<ResourceDescriptor>& descriptor);

		ResourceStateTracking& GetStateTracking();
		const ResourceStateTracking& GetStateTracking() const;

		void SetGpuPending(Event event);
		bool IsGpuPending() const;
		void SyncGpu();

	protected:
		Event m_GpuPendingEvent;
		ResourceStateTracking m_StateTracking;
		std::unordered_map<uint64_t, Ref<ResourceDescriptor>> m_Descriptors;
		ComPtr<ID3D12Resource> m_Resource;
	};
}