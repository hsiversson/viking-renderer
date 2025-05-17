#pragma once
#include "rendercommon.h"

namespace vkr::Render
{
	struct BufferDesc
	{

	};

	class Buffer
	{
	public:
		Buffer();
		~Buffer();

		bool Init(const BufferDesc& desc);

		// How do we handle resource views? (SRV, UAV)
		// Separate class, or do we include here?
		// Each resource can have multiple views.
		ID3D12Resource* GetD3DResource() { return m_Resource.Get(); }

	private:
		ComPtr<ID3D12Resource> m_Resource; // do we generalize the resources between texture/buffer? We need to keep track of resource state etc, so would be annoying to duplicate.
	};
}