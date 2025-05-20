#pragma once
#include "render/deviceobject.h"
#include "render/rendercommon.h"

namespace vkr::Render
{
	struct BufferDesc
	{
		unsigned int Size;
		bool bWriteOnCPU = true;
		bool bWriteOnGPU = false;
	};

	class Buffer : public DeviceObject
	{
	public:
		Buffer(Device& device);
		~Buffer();

		bool Init(ID3D12Resource* resource);

		// How do we handle resource views? (SRV, UAV)
		// Separate class, or do we include here?
		// Each resource can have multiple views.
		ID3D12Resource* GetD3DResource() { return m_Resource.Get(); }

	private:
		ComPtr<ID3D12Resource> m_Resource; // do we generalize the resources between texture/buffer? We need to keep track of resource state etc, so would be annoying to duplicate.
	};
}